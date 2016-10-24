#include "NuclearPotential.h"
#include "NuclearFunction.h"
#include "mrchem.h"

NuclearPotential::NuclearPotential(double build_prec, Nuclei &nucs)
        : Potential(),
          project(*MRA, -1.0),
          nuc_func(nucs, build_prec) {
}

NuclearPotential::~NuclearPotential() {
}

void NuclearPotential::setup(double prec) {
    Timer timer;
    Potential::setup(prec);
    this->project.setPrecision(prec);
    if (this->real == 0) {
        this->real = this->grid();
        this->project(*this->real, this->nuc_func);
        this->imag = 0;
    } else {
        NOT_IMPLEMENTED_ABORT;
    }
    timer.stop();
    int n = getNNodes();
    double t = timer.getWallTime();
    TelePrompter::printTree(0, "Nuclear potential", n, t);
}

void NuclearPotential::clear() {
    this->project.setPrecision(-1.0);
    Potential::clear();
}
