// Cycle complet sur la sonde simulée, en CSV (pour docs/cycle-chauffe-simule.svg) :
// chauffe de la fiche, Ri mesurée par injection, pompe, lambda.
//   make trace
#include <stdio.h>

#include "lsu_driver.h"
#include "sim_hardware.h"

int main() {
  LsuCircuit circuit;
  LsuControlConfig control;
  HeaterConfig heaterCfg;
  LsuController ctl(circuit, control, heaterCfg);
  SimLsuHardware sim(circuit);
  LsuOutputs out = {false, ctl.pumpDacVolts(0.0f) / circuit.vcc_V, 0.0f};
  const float dt = 1.0f / control.sampleRate_Hz;
  const long perRow = (long)(0.1f / dt + 0.5f);  // une ligne toutes les 100 ms

  printf("t_s,phase,Uh_eff_V,Ri_mesuree_ohm,Ri_vraie_ohm,Nernst_V,Ip_mA,lambda_mesure,lambda_vrai,pompe,pret\n");
  for (long k = 0; k <= (long)(60.0f / dt + 0.5f); ++k) {
    out = ctl.step(sim.step(out, dt));
    if (k % perRow == 0) {
      const LsuStatus& s = ctl.status();
      printf("%.2f,%s,%.3f,%.1f,%.1f,%.4f,%.4f,%.4f,%.4f,%d,%d\n", k * dt, heaterPhaseShort(s.phase), s.heater_V,
             s.esr_Ohm, sim.trueEsr_Ohm(), s.nernst_V, s.pumpMeasured_mA, s.valid ? s.lambda : 0.0f,
             sim.trueLambda(), s.pumpActive ? 1 : 0, s.valid ? 1 : 0);
    }
  }
  return 0;
}
