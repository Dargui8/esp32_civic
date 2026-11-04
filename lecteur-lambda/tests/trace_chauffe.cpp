// Cycle de chauffe sur la sonde simulée, en CSV (pour docs/cycle-chauffe-simule.svg).
//   make trace
#include <stdio.h>

#include "heater.h"
#include "lsu_adv.h"
#include "sensor_sim.h"

int main() {
  SimFrontEnd sim;
  HeaterController heater;
  const float dt = 0.01f;
  printf("t_s,phase,Uh_eff_V,Ri_ohm,temperature_C,pret\n");
  for (int k = 0; k <= 6000; ++k) {  // 60 s
    const FrontEndSample s = sim.read();
    heater.update(dt, s.ri_Ohm, s.ubat_V);
    sim.heaterApplied(heater.appliedVoltage_V(), dt);
    if (k % 10 == 0) {
      printf("%.2f,%s,%.3f,%.1f,%.1f,%d\n", k * dt, heaterPhaseShort(heater.phase()),
             heater.appliedVoltage_V(), sim.trueRi_Ohm(), sim.sensorTemp_C(), heater.ready() ? 1 : 0);
    }
  }
  return 0;
}
