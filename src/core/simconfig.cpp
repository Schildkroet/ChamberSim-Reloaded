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

#include "simconfig.h"

#include "loadbuilder.h"
#include "numeric.h"
#include "shotparts.h"

#include <algorithm>

namespace csr
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
    in.grooveDiameter_mm = c.number(u"Dz");
    in.sebert = c.number(u"sebert", in.sebert);
    in.startPressure_bar = loadStartPressure_bar(c);
    in.primerOffset_bar = csr::primerOffset_bar(c);  // physical model only
    in.maxPressure_bar = c.number(u"pMaxZul");
    in.handgun = lf.caliberFile.text(u"cippdf").contains(u"/tab-iv/") || lf.caliberFile.text(u"ciptype").contains(u"pistol");
    in.shotshell = c.flag(u"shotshell");
    if (in.shotshell)
    {
        const ShotFactors f = shotFactorsOf(c);
        in.shotCapacityScale = f.capacity;
        in.shotEnergyScale = f.energy;
    }

    if (!in.shotshell)
    {
        // the factor of the caliber (data/physical/calibers.json: pistol, revolver and rifle cartridges); the factor of the class (auto, rimmed)
        // exists for the handgun cartridges only
        const double head = lf.caliberFile.number(u"P1");
        in.caliberBurnScale =
            physicalCaliberScale(lf.caliberFile.text(u"cipname"), in.handgun && head > 0.0 ? lf.caliberFile.number(u"R1") / head : 0.0);
    }

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

}  // namespace csr
