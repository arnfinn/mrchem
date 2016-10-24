#ifndef EXCHANGEPOTENTIAL_H
#define EXCHANGEPOTENTIAL_H

#include "ExchangeOperator.h"
#include "OrbitalVector.h"

class ExchangePotential : public ExchangeOperator {
public:
    ExchangePotential(double build_prec,
                      OrbitalVector &phi,
                      double x_fac = 1.0);
    virtual ~ExchangePotential();

    virtual void rotate(Eigen::MatrixXd &U);

    virtual void setup(double prec);
    virtual void clear();

    virtual Orbital* operator() (Orbital &phi_p);
    virtual Orbital* adjoint(Orbital &phi_p);

    using QMOperator::operator();
    using QMOperator::adjoint;
protected:
    OrbitalVector exchange_0;  ///< Set to keep precomputed exchange orbitals

    Orbital* calcExchange(Orbital &phi_p);
    Orbital* testPreComputed(Orbital &phi_p);
    void calcInternalExchange();
    int calcInternal(int i);
    int calcInternal(int i, int j);
};

#endif // EXCHANGEPOTENTIAL_H
