#ifndef DENSITY_H
#define DENSITY_H

#include "constants.h"

template<int D> class FunctionTree;

class Density {
public:
    Density(bool s = false);
    Density(const Density &rho);
    virtual ~Density();
    void clear();

    void setIsSpinDensity(bool s) { this->is_spin = s; }
    bool isSpinDensity() const { return this->is_spin; }
    int getNNodes(int type = Density::Total) const;

    void setDensity(int s, FunctionTree<3> *rho);

    void allocTotal();
    void allocSpin();
    void allocAlpha();
    void allocBeta();

    bool hasTotal() const { if (this->dens_t == 0) return false; return true; }
    bool hasSpin() const { if (this->dens_s == 0) return false; return true; }
    bool hasAlpha() const { if (this->dens_a == 0) return false; return true; }
    bool hasBeta() const { if (this->dens_b == 0) return false; return true; }

    FunctionTree<3> &total() { return *this->dens_t; }
    FunctionTree<3> &spin() { return *this->dens_s; }
    FunctionTree<3> &alpha() { return *this->dens_a; }
    FunctionTree<3> &beta() { return *this->dens_b; }

    const FunctionTree<3> &total() const { return *this->dens_t; }
    const FunctionTree<3> &spin() const { return *this->dens_s; }
    const FunctionTree<3> &alpha() const { return *this->dens_a; }
    const FunctionTree<3> &beta() const { return *this->dens_b; }

    void send_Density(int dest, int tag);
    void Rcv_Density(int source, int tag);

    enum Spin { Total, Spin, Alpha, Beta };

protected:
    bool is_spin;
    FunctionTree<3> *dens_t;
    FunctionTree<3> *dens_s;
    FunctionTree<3> *dens_a;
    FunctionTree<3> *dens_b;
};

#endif // DENSITY_H

