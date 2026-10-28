/*
 * This file is part of ChamberSim Reloaded.
 *
 * Copyright (C) 2026 Schildkroet
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "report.h"

#include "loadbuilder.h"
#include "numeric.h"
#include "reportformat.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QRegularExpression>

#include <cmath>

using namespace Qt::StringLiterals;

namespace csr::report
{

Value Value::ofNumber(double base, units::Quantity q)
{
    Value v;
    v.kind = Kind::Number;
    v.number = base;
    v.quantity = q;
    return v;
}

Value Value::ofFraction(double fraction)
{
    Value v = ofNumber(fraction);
    v.percent = true;
    return v;
}

Value Value::ofText(QString text)
{
    Value v;
    v.kind = Kind::Text;
    v.text = std::move(text);
    return v;
}

Value Value::ofImage(QByteArray png)
{
    Value v;
    v.kind = Kind::Image;
    v.png = std::move(png);
    return v;
}

namespace
{

QString automatic(const Value &v, const RenderOptions &o)
{
    if (v.percent) return formatNumber(v.number, u"0.0 %"_s);
    if (v.quantity == units::Quantity::None) return formatNumber(v.number, u"0.####"_s);
    const units::Unit &u = o.unitFor ? o.unitFor(v.quantity) : units::unitsOf(v.quantity).front();
    return formatNumber(units::toDisplay(v.number, u), u"0"_s + (u.decimals > 0 ? u"."_s + QString(u.decimals, u'0') : QString())) + u' ' +
           QString::fromUtf8(u.symbol);
}

}  // namespace

QString expandPragmas(const QString &text, const Data &data, const RenderOptions &options)
{
    static const QRegularExpression re(u"~~\\s*([A-Za-z_][A-Za-z0-9_.]*)\\s*(?:\\(\\s*\"([^\"]*)\"\\s*\\))?\\s*~~"_s);
    QString out;
    qsizetype pos = 0;
    for (auto it = re.globalMatch(text); it.hasNext();)
    {
        const auto m = it.next();
        out += text.mid(pos, m.capturedStart() - pos);
        pos = m.capturedEnd();
        const QString key = m.captured(1);
        const bool hasFormat = m.capturedStart(2) >= 0;
        const Value *v = data.find(key);
        if (!v)
        {
            out += u"<color red>[%1?]</color>"_s.arg(key);
            continue;
        }
        switch (v->kind)
        {
        case Value::Kind::Text: out += v->text; break;
        case Value::Kind::Image: out += u"{{%1%2}}"_s.arg(key, hasFormat ? m.captured(2) : QString()); break;
        case Value::Kind::Number: out += hasFormat ? formatNumber(v->number, m.captured(2)) : automatic(*v, options); break;
        }
    }
    return out + text.mid(pos);
}

QString renderReport(const QString &templateText, const Data &data, const RenderOptions &options)
{
    const QString expanded = expandPragmas(templateText, data, options);
    const ImageResolver resolver = [&](const QString &name) -> ImageSource
    {
        const Value *v = data.find(name);
        if (!v || v->kind != Value::Kind::Image || v->png.size() < 24) return {};
        // natural size from the PNG header (IHDR: width and height as big-endian 32 bit numbers)
        const auto be32 = [&](int at)
        {
            const auto *p = reinterpret_cast<const unsigned char *>(v->png.constData()) + at;
            const quint32 v = (quint32{p[0]} << 24) | (quint32{p[1]} << 16) | (quint32{p[2]} << 8) | quint32{p[3]};
            return v > 100'000 ? 0 : static_cast<int>(v);  // a broken header: no natural size
        };
        return {u"data:image/png;base64,"_s + QString::fromLatin1(v->png.toBase64()), be32(16), be32(20)};
    };
    return u"<html><body>"_s + wikiToHtml(expanded, resolver, options.pageWidth) + u"</body></html>"_s;
}

Data buildData(const LoadFile &lf, const SimInput &in, const SimResult &r, const FileInfo &file)
{
    using units::Quantity;
    Data d;
    const auto num = [&](const QString &key, double v, Quantity q = Quantity::None) { d.set(key, Value::ofNumber(v, q)); };
    const auto txt = [&](const QString &key, const QString &v) { d.set(key, Value::ofText(v)); };

    // ---- file ----
    const QFileInfo fi(file.path);
    txt(u"file.caption"_s, file.path.isEmpty() ? lf.title : fi.completeBaseName());
    txt(u"file.name"_s, file.path.isEmpty() ? QString() : fi.fileName());
    txt(u"file.path"_s, file.path);
    txt(u"app.version"_s, QCoreApplication::applicationVersion());  // the program that made the report
    // measurement results: a whole report section (heading, table, chart) filled in by the application, which
    // knows the user's units; empty when the load has no measurements
    txt(u"measurement.section"_s, QString());

    // ---- work: the user's inputs ----
    const auto &c = lf.caliber;
    const auto &p = lf.projectile;
    const auto &q = lf.propellant;
    txt(u"work.CaliberName"_s, c.text(u"CaliberName"));
    num(u"work.Dz"_s, c.number(u"Dz"), Quantity::Length);
    num(u"work.pmaxZul"_s, c.number(u"pMaxZul"), Quantity::Pressure);
    num(u"work.Aeff"_s, c.number(u"Aeff"), Quantity::Area);
    num(u"work.caselen"_s, c.number(u"caselen"), Quantity::Length);
    num(u"work.casevol"_s, c.number(u"casevol"), Quantity::Volume);
    num(u"work.oal"_s, c.number(u"oal"), Quantity::Length);
    const double effVolume = r.ok() ? r.effectiveCaseVolume_cm3 : in.caseVolume_cm3 - bulletIntrusion_m3(in) * 1e6;
    num(u"work.Vb"_s, effVolume, Quantity::Volume);
    num(u"work.sebert"_s, in.sebert);  // the load's value (the physical model uses it only for recoil and propellant impulse)
    num(u"work.xe"_s, lf.gun.number(u"xe"), Quantity::Length);
    num(u"work.xeEff"_s, lf.gun.number(u"xe") - in.startPosition_mm(), Quantity::Length);
    num(u"work.twistlen"_s, lf.gun.number(u"twistlen"), Quantity::Length);
    num(u"work.gjump"_s, lf.gun.number(u"gjump"), Quantity::Length);  // free run of the bullet to the lands
    num(u"work.ps"_s, c.number(u"ps"), Quantity::Pressure);

    txt(u"work.ProjectileName"_s, p.text(u"ProjectileName"));
    num(u"work.caliber"_s, p.number(u"Dbul") / kMmPerInch);  // numeric designation in inches
    num(u"work.Dbul"_s, p.number(u"Dbul"), Quantity::Length);
    num(u"work.mp"_s, p.number(u"mp"), Quantity::BulletMass);
    num(u"work.glen"_s, p.number(u"glen"), Quantity::Length);
    num(u"work.gdepth"_s, p.number(u"gdepth"), Quantity::Length);
    const int tailType = toIntClamped(p.number(u"gTailType"), 0, 2);
    // the guided seating depth is the contact length of the cylindrical part: a boat tail does not guide
    num(u"work.gdepthc"_s, std::max(0.0, p.number(u"gdepth") - (tailType == 2 ? p.number(u"gtailh") : 0.0)), Quantity::Length);
    txt(u"work.gtailtype"_s, tailType == 1 ? u"hollowbase"_s : tailType == 2 ? u"boattail"_s : u"flatbase"_s);
    num(u"work.gtailh"_s, p.number(u"gtailh"), Quantity::Length);
    num(u"work.gtaildiaA"_s, p.number(u"gTailDiaA"), Quantity::Length);
    num(u"work.gtaildiaB"_s, p.number(u"gTailDiaB"), Quantity::Length);
    txt(u"work.gmaterial"_s, p.text(u"gmaterial"));
    txt(u"work.gUBCS"_s, p.text(u"gUBCS"));
    num(u"work.g1bc"_s, p.number(u"g1bc"));
    num(u"work.g7bc"_s, p.number(u"g7bc"));

    txt(u"work.PropellantName"_s, (q.text(u"mname") + u' ' + q.text(u"pname")).trimmed());
    num(u"work.Qex"_s, q.number(u"Qex"));
    num(u"work.k"_s, q.number(u"k"));
    num(u"work.Ba"_s, q.number(u"Ba"));
    num(u"work.a0"_s, q.number(u"a0"));
    num(u"work.z1"_s, q.number(u"z1"));
    num(u"work.z2"_s, q.number(u"z2"));
    num(u"work.pc"_s, q.number(u"pc"), Quantity::Density);
    num(u"work.pcd"_s, q.number(u"pcd"), Quantity::Density);
    num(u"work.pt"_s, q.number(u"pt"), Quantity::Temperature);
    num(u"work.mc"_s, in.charge_g, Quantity::BulletMass);  // the charge of the selected ladder curve
    num(u"work.laddercnt"_s, q.number(u"laddercnt"));
    num(u"work.laddermc"_s, q.number(u"laddermc"), Quantity::BulletMass);

    // ---- result: the simulation (only when it ran) ----
    if (r.ok())
    {
        num(u"result.MaxPressure"_s, r.maxPressure_bar, Quantity::Pressure);
        num(u"result.MuzzlePressure"_s, r.muzzlePressure_bar, Quantity::Pressure);
        num(u"result.MuzzleEnergy"_s, r.muzzleEnergy_J, Quantity::Energy);
        num(u"result.MuzzleVelocity"_s, r.muzzleVelocity_ms, Quantity::Velocity);
        num(u"result.MuzzleTime"_s, r.muzzleTime_ms, Quantity::Time);
        num(u"result.BulletLeadTime"_s, r.bulletLeadTime_ms, Quantity::Time);
        if (r.optimalBarrelNode >= 0.0)
        {
            const int whole = static_cast<int>(r.optimalBarrelNode);
            num(u"result.OptimalBarrelTime"_s, r.optimalBarrelTime_ms, Quantity::Time);
            txt(u"result.OptimalBarrelNode"_s, u"#%1%2"_s.arg(whole).arg(r.optimalBarrelNode > whole ? u" ½"_s : QString()));
        }
        else
        {
            txt(u"result.OptimalBarrelTime"_s, u"–"_s);
            txt(u"result.OptimalBarrelNode"_s, u"–"_s);
        }
        d.set(u"result.LoadRatio"_s, Value::ofFraction(r.loadRatio));
        d.set(u"result.BurnRatio"_s, Value::ofFraction(r.burntFraction));
        if (r.burnoutInBarrel)
        {
            num(u"result.BurnOutTime"_s, r.points[r.indexBurnout].t_ms, Quantity::Time);
            num(u"result.BurnOutPos"_s, r.points[r.indexBurnout].x_mm, Quantity::Length);
        }
        d.set(u"result.Efficiency"_s, Value::ofFraction(r.efficiency));
        num(u"result.IPSCFactor"_s, r.ipscFactor);
        num(u"result.MIPFactor"_s, r.mipFactor);
        num(u"result.ImpulseProjectile"_s, r.projectileImpulse_Ns, Quantity::Impulse);
        num(u"result.ImpulsePropellant"_s, r.propellantImpulse_Ns, Quantity::Impulse);
        num(u"result.Impulse"_s, r.recoilImpulse_Ns, Quantity::Impulse);
        if (in.charge_g > 0.0) num(u"result.OutflowVelocity"_s, r.gasImpulse_Ns / (in.charge_g * 1e-3), Quantity::Velocity);
        num(u"result.RecoilForce"_s, r.meanRecoilForce_N, Quantity::Force);
        if (in.gunWeight_kg > 0.0)
        {
            num(u"result.RecoilVelocity"_s, r.recoilVelocity_ms, Quantity::Velocity);
            num(u"result.RecoilEnergy"_s, r.recoilEnergy_J, Quantity::Energy);
        }
        else
        {  // needs the gun weight
            txt(u"result.RecoilVelocity"_s, u"–"_s);
            txt(u"result.RecoilEnergy"_s, u"–"_s);
        }
    }

    // ---- caliber: the database record that came with the load ----
    txt(u"caliber.pname"_s, lf.caliberFile.text(u"cipname"));
    txt(u"caliber.mname"_s, lf.caliberFile.text(u"altname"));
    return d;
}

QString defaultTemplate()
{
    return uR"(====== ~~file.caption~~ ======

~~work.CaliberName~~ · ~~work.ProjectileName~~ · ~~work.PropellantName~~, ~~work.mc~~

===== Input data =====
^ Cartridge and gun ^^ Projectile ^^
| Case length (L3) | ~~work.caselen~~ | Diameter | ~~work.Dbul~~ |
| Overall length (L6) | ~~work.oal~~ | Mass | ~~work.mp~~ |
| Case volume | ~~work.casevol~~ | Length | ~~work.glen~~ |
| Combustion chamber | ~~work.Vb~~ | Seating depth | ~~work.gdepth~~ |
| Barrel length | ~~work.xe~~ | Base type | ~~work.gtailtype~~ |
| Twist length | ~~work.twistlen~~ | Bullet jump | ~~work.gjump~~ |
| Sebert factor | ~~work.sebert~~ | Initial pressure | ~~work.ps~~ |
| Max. allowed pressure | ~~work.pmaxZul~~ | G1 ballistic coefficient | ~~work.g1bc~~ |

^ Propellant ^^ Charge and temperature ^^
| Powder | ~~work.PropellantName~~ | Charge | ~~work.mc~~ |
| Burn coefficient (Ba) | ~~work.Ba~~ | Temperature | ~~work.pt~~ |
| Transition limits | ~~work.z1("0.000")~~ / ~~work.z2("0.000")~~ | Bulk density | ~~work.pcd~~ |

===== Results =====
^ Pressures and energy ^^ Speeds and recoil ^^
| Maximum pressure | ~~result.MaxPressure~~ | Muzzle velocity | ~~result.MuzzleVelocity~~ |
| Muzzle pressure | ~~result.MuzzlePressure~~ | IPSC / MIP factor | ~~result.IPSCFactor("0")~~ / ~~result.MIPFactor("0")~~ |
| Muzzle energy | ~~result.MuzzleEnergy~~ | Barrel time | ~~result.MuzzleTime~~ |
| Load ratio | ~~result.LoadRatio~~ | Recoil impulse | ~~result.Impulse~~ |
| Burnt propellant | ~~result.BurnRatio~~ | Recoil energy | ~~result.RecoilEnergy~~ |
| Efficiency | ~~result.Efficiency~~ | Gas outflow velocity | ~~result.OutflowVelocity~~ |
| Bullet lead time (BLT) | ~~result.BulletLeadTime~~ | Optimal barrel time (OBT) | ~~result.OptimalBarrelTime~~, node ~~result.OptimalBarrelNode~~ |

<pagebreak>
===== Diagrams =====
{{result.diagramCurves.png?100%}}

{{caliber.drawing.png?27%}} {{caliber.chamber.png?27%}} {{result.diagramPropellant.png?40%}}

~~measurement.section~~

----
<size 80%>The results are calculated and can differ considerably from reality. Compare every load with the current
loading tables of the component manufacturers. Use at your own risk.
Created with ChamberSim Reloaded ~~app.version~~.</size>
)"_s;
}

}  // namespace csr::report
