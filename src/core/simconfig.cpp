#include "simconfig.h"

#include "numeric.h"

#include <algorithm>

namespace grt
{

SimInput SimInput::fromLoad(const LoadFile &lf)
{
    const auto &c = lf.caliber;
    const auto &g = lf.gun;
    const auto &p = lf.projectile;
    const auto &q = lf.propellant;

    SimInput in;
    in.caseLength_mm = c.number(u"caselen");
    in.caseVolume_cm3 = c.number(u"casevol");
    in.boreArea_mm2 = c.number(u"Aeff");
    in.sebert = c.number(u"sebert", in.sebert);
    in.startPressure_bar = c.number(u"ps", in.startPressure_bar);
    in.maxPressure_bar = c.number(u"pMaxZul");
    in.shotshell = c.flag(u"shotshell");

    in.barrelLength_mm = g.number(u"xe");
    in.twistLength_mm = g.number(u"twistlen");
    in.gunWeight_kg = g.number(u"mgun");
    in.bulletJump_mm = std::max(g.number(u"gjump"), 0.0);

    in.bulletDiameter_mm = p.number(u"Dbul");
    in.bulletMass_g = p.number(u"mp");
    in.bulletLength_mm = p.number(u"glen");
    in.seatingDepth_mm = p.number(u"gdepth");
    in.tailLength_mm = p.number(u"gtailh");
    in.tailDiameterA_mm = p.number(u"gTailDiaA");
    in.tailDiameterB_mm = p.number(u"gTailDiaB");
    in.tailType = toIntClamped(p.number(u"gTailType"), 0, 2);
    in.extraFriction_bar = p.number(u"gfriction");
    in.extraFrictionUnlocked = p.flag(u"gfrictionUnlock");

    in.vivacityBa = q.number(u"Ba");
    in.heatQex_kJkg = q.number(u"Qex");
    in.kappa = q.number(u"k", in.kappa);
    in.a0 = q.number(u"a0");
    in.a1 = q.number(u"a1");
    in.z1 = q.number(u"z1");
    in.z2 = q.number(u"z2");
    in.covolumeEta = q.number(u"eta", in.covolumeEta);
    in.density_kgm3 = q.number(u"pc", in.density_kgm3);
    in.bulkDensity_kgm3 = q.number(u"pcd", in.bulkDensity_kgm3);
    in.temperature_C = q.number(u"pt", in.temperature_C);
    in.coldCoeff = q.number(u"tcc");
    in.hotCoeff = q.number(u"tch");
    in.charge_g = q.number(u"mc");
    in.ladderCount = toIntClamped(q.number(u"laddercnt", 1.0), 0, 1000);  // the solver limits it further
    in.ladderStep_g = q.number(u"laddermc");
    in.physicalBurn = readPhysicalBurn(q);
    return in;
}

}  // namespace grt
