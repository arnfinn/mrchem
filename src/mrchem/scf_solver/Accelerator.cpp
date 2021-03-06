#include <Eigen/Dense>

#include "Accelerator.h"
#include "Orbital.h"
#include "OrbitalVector.h"
#include "eigen_disable_warnings.h"

using namespace std;
using namespace Eigen;

extern MultiResolutionAnalysis<3> *MRA; // Global MRA

Accelerator::Accelerator(int max, int min, bool sep)
        : add(-1.0, MRA->getMaxScale()),
          minHistory(min),
          maxHistory(max),
          sepOrbitals(sep) {
    if (this->minHistory < 0) this->minHistory = 0;
    if (this->maxHistory < this->minHistory) MSG_ERROR("Invalid argument");
}

/** Delete all orbitals and Fock matrices in history.
  *
  * Leaves the accelerator in the same state as after construction,
  * ready for use in the next optimization. */
void Accelerator::clear() {
    while (this->orbitals.size() > 0) {
        if (this->orbitals[0] != 0) {
            delete this->orbitals[0];
        }
        this->orbitals.pop_front();
    }
    while (this->dOrbitals.size() > 0) {
        if (this->dOrbitals[0] != 0) {
            delete this->dOrbitals[0];
        }
        this->dOrbitals.pop_front();
    }
    while (this->fock.size() > 0) {
        this->fock.pop_front();
    }
    while (this->dFock.size() > 0) {
        this->dFock.pop_front();
    }
    clearLinearSystem();
}

/** Delete the matrices and vectors used to compute the next step.
  *
  * The accelerator is now ready to move to the next iteration.*/
void Accelerator::clearLinearSystem() {
    int N = this->A.size();
    for (int n = 0; n < N; n++) {
        if (this->A[n] != 0) delete this->A[n];
        if (this->b[n] != 0) delete this->b[n];
        if (this->c[n] != 0) delete this->c[n];
    }
    this->A.clear();
    this->b.clear();
    this->c.clear();
}

/** Rotate iterative history.
 *
 * To keep phases and orbital ordering consistent one can apply
 * the latest orbital rotation to the entire orbital history.
 * Not recommended as rotations are expensive. Instead one should
 * clear history and start over. Option to rotate the last orbital
 * set or not. */
void Accelerator::rotate(const MatrixXd &U, bool rotAll) {
    Timer timer;
    int nOrbs = this->orbitals.size() - 1;
    int nFock = this->fock.size() - 1;
    if (rotAll) {
        nOrbs += 1;
        nFock += 1;
    }
    if (nOrbs <= 0) {
        return;
    }
    for (int i = 0; i < nOrbs; i++) {
        OrbitalVector &phi = *this->orbitals[i];
        OrbitalVector &dPhi = *this->dOrbitals[i];
        this->add.rotate(phi, U);
        this->add.rotate(dPhi, U);
    }
    for (int i = 0; i < nFock; i++) {
        MatrixXd &F = this->fock[i];
        MatrixXd &dF = this->dFock[i];
        F = U*F*U.transpose();
        dF = U*dF*U.transpose();
    }

    timer.stop();
    double t = timer.getWallTime();
    TelePrompter::printDouble(0, "Rotating iterative subspace", t);
}

/** Update iterative history with the latest orbitals and updates
 *
 * The new orbitals are moved from the input sets and into the newly
 * constructed local sets. This means that the input sets contain a
 * bunch of zero pointers after this routine. To get the orbitals
 * back, use calcUpdates or getOrbitals. If F and dF are given as
 * input, the matrices are included in the subspace. If the length
 * of the history exceed maxHistory the oldest orbitals are discarded.
 */
void Accelerator::push_back(OrbitalVector &phi,
                            OrbitalVector &dPhi,
                            MatrixXd *F,
                            MatrixXd *dF) {
    Timer timer;
    int nHistory = this->orbitals.size();
    bool historyIsFull = false;
    if (nHistory >= this->maxHistory) {
        historyIsFull = true;
    }
    if (F != 0) {
        if (dF == 0) {
            MSG_ERROR("Need to give both F and dF");
        }
        if (this->fock.size() != nHistory) {
            MSG_ERROR("Size mismatch orbitals vs matrices");
        }
    }

    if (this->orbitals.size() > this->maxHistory - 1) {
        delete this->orbitals[0];
        this->orbitals.pop_front();
    }
    if (this->dOrbitals.size() > this->maxHistory - 1) {
        delete this->dOrbitals[0];
        this->dOrbitals.pop_front();
    }
    if (this->fock.size() > this->maxHistory - 1) {
        this->fock.pop_front();
    }
    if (this->dFock.size() > this->maxHistory - 1) {
        this->dFock.pop_front();
    }
    if (not verifyOverlap(phi)) {
        println(0, " Clearing accelerator");
        this->clear();
    }

    OrbitalVector *new_phi = new OrbitalVector(phi);
    OrbitalVector *new_dPhi = new OrbitalVector(dPhi);
    *new_phi = phi;
    *new_dPhi = dPhi;
    phi.clear(false);
    dPhi.clear(false);

    this->orbitals.push_back(new_phi);
    this->dOrbitals.push_back(new_dPhi);
    if (F != 0) this->fock.push_back(*F);
    if (dF != 0) this->dFock.push_back(*dF);

    timer.stop();
    double t = timer.getWallTime();
    TelePrompter::printDouble(0, "Push back orbitals", t);
}

/** Verify that the orbital overlap between the two last iterations
  * is positive.
  *
  * If the overlap is negative a sign change of the corresponding
  * orbitals in the entire history is required (unless the history
  * is cleared).
  */
bool Accelerator::verifyOverlap(OrbitalVector &phi) {
    bool verified = true;
    int nHistory = this->orbitals.size() - 1;
    if (nHistory > 0) {
        for (int i = 0; i < phi.size(); i++) {
            Orbital &phi_i = phi.getOrbital(i);
            Orbital &last_i = this->orbitals[nHistory]->getOrbital(i);
            double sqNorm = phi_i.getSquareNorm();
            complex<double> overlap = phi_i.dot(last_i);
            if (overlap.imag() > MachineZero) NOT_IMPLEMENTED_ABORT;
            if (overlap.real() < 0.5*sqNorm) {
                TelePrompter::printDouble(0, "Overlap not verified ", overlap.real());
                verified = false;
            }
        }
    }
    return verified;
}

/** Calculates the new orbitals and updates based on history information
 *
 * Solves the linear system of equations \f$ Ac = b \f$ that defines the
 * coefficients \f$ c \f$ of the next step
 * copies the resulting orbitals into the output sets. If length of history
 * is less than minHistory, the latest orbitals are copied directly into
 * the output sets without solving the linear problem. Existing input
 * orbitals and updates are replaced by new ones. */
void Accelerator::accelerate(double prec,
                             OrbitalVector &phi,
                             OrbitalVector &dPhi,
                             MatrixXd *F,
                             MatrixXd *dF) {
    this->add.setPrecision(prec);
    TelePrompter::printHeader(0, "Iterative subspace accelerator");

    Timer timer;
    this->push_back(phi, dPhi, F, dF);

    int nHistory = this->orbitals.size() - 1;
    if (nHistory <= this->minHistory) {
        copyOrbitals(phi);
        copyOrbitalUpdates(dPhi);
    } else {
        setupLinearSystem();
        solveLinearSystem();
        expandSolution(phi, dPhi, F, dF);
        clearLinearSystem();
    }
    timer.stop();
    TelePrompter::printFooter(0, timer, 2);
    this->add.setPrecision(-1.0);
}

void Accelerator::solveLinearSystem() {
    Timer timer;
    this->c.clear();
    int N = this->A.size();
    for (int n = 0; n < N; n++) {
        VectorXd *tmpC = new VectorXd;
        *tmpC = this->A[n]->colPivHouseholderQr().solve(*this->b[n]);
        this->c.push_back(tmpC);
    }
    timer.stop();
    double t = timer.getWallTime();
    TelePrompter::printDouble(0, "Solve linear system", t);
}


/** Copy orbitals at the given point in history into the given set.
 *
 * Input set is overwritten if it contains orbitals. Counts backwards,
 * zero history input returns latest orbital set. */
void Accelerator::copyOrbitals(OrbitalVector &phi, int nHistory) {
    Timer timer;
    int totHistory = this->orbitals.size();
    if (nHistory >= totHistory or nHistory < 0) {
        MSG_FATAL("Requested orbitals unavailable");
    }
    int n = totHistory - 1 - nHistory;
    MatrixXd I = MatrixXd::Identity(phi.size(), phi.size());
    this->add.rotate(phi, I, *this->orbitals[n]);

    timer.stop();
    double t = timer.getWallTime();
    TelePrompter::printDouble(0, "Copy orbitals", t);
}

/** Copy orbital updates at the given point in history into the given set.
 *
 * Input set is overwritten if it contains orbitals. Counts backwards,
 * zero history input returns latest orbital set. */
void Accelerator::copyOrbitalUpdates(OrbitalVector &dPhi, int nHistory) {
    Timer timer;
    int totHistory = this->dOrbitals.size();
    if (nHistory >= totHistory or nHistory < 0) {
        MSG_FATAL("Requested orbitals unavailable");
    }
    int n = totHistory - 1 - nHistory;
    MatrixXd I = MatrixXd::Identity(dPhi.size(), dPhi.size());
    this->add.rotate(dPhi, I, *this->dOrbitals[n]);

    timer.stop();
    double t = timer.getWallTime();
    TelePrompter::printDouble(0, "Copy orbital updates", t);
}

/** Replaces the orbital set from a given point in history.
 *
 * Deletes the old orbital set and copies the new. Counts backwards,
 * zero input returns latest orbital set. */
void Accelerator::replaceOrbitals(OrbitalVector &phi, int nHistory) {
    NOT_IMPLEMENTED_ABORT;
//    int totHistory = this->orbitals.size();
//    if (nHistory >= totHistory or nHistory < 0) {
//        MSG_FATAL("Requested orbitals unavailable");
//    }
//    int n = totHistory - 1 - nHistory;

//    string oldName = this->orbitals[n]->getName();
//    OrbitalSet *orbSet = new OrbitalSet(oldName, orbs);
//    *orbSet = orbs;
//    delete this->orbitals[n];
//    this->orbitals[n] = orbSet;
}

/** Replaces the orbital update set from a given point in history
 *
 * Deletes the old orbital set and copies the new. Counts backwards,
 * zero input returns latest orbital set. */
void Accelerator::replaceOrbitalUpdates(OrbitalVector &dPhi, int nHistory) {
    NOT_IMPLEMENTED_ABORT;
//    int totHistory = this->dOrbitals.size();
//    if (nHistory >= totHistory or nHistory < 0) {
//        MSG_FATAL("Requested orbitals unavailable");
//    }
//    int n = totHistory - 1 - nHistory;

//    string oldName = this->dOrbitals[n]->getName();
//    OrbitalSet *orbSet = new OrbitalSet(oldName, dOrbs);
//    *orbSet = dOrbs;
//    delete this->dOrbitals[n];
//    this->dOrbitals[n] = orbSet;
}

/** Creates the final A matrices and b vectors.
 *
 * Input vectors may include nOrbs + 1 entries, one extra from the Fock matrix.
 * If orbitals are not separated these are added up to one final A matrix and
 * b vector, otherwise all individual entries are kept. */
void Accelerator::sortLinearSystem(vector<MatrixXd *> &A_matrices,
                                   vector<VectorXd *> &b_vectors) {
    int nOrbs = b_vectors.size();
    int nHist = b_vectors[0]->size();

    if (this->sepOrbitals) {
        for (int i = 0; i < nOrbs; i++) {
            MatrixXd *tmpA = new MatrixXd(*A_matrices[i]);
            VectorXd *tmpB = new VectorXd(*b_vectors[i]);
            this->A.push_back(tmpA);
            this->b.push_back(tmpB);
        }
    } else {
        MatrixXd *tmpA = new MatrixXd(nHist,nHist);
        VectorXd *tmpB = new VectorXd(nHist);
        tmpA->setZero();
        tmpB->setZero();
        for (int i = 0; i < nOrbs; i++) {
            *tmpA += *A_matrices[i];
            *tmpB += *b_vectors[i];
        }
        this->A.push_back(tmpA);
        this->b.push_back(tmpB);
    }
}

/** Prints the number of trees and nodes kept in the iterative history */
/*
int Accelerator::printTreeSizes() const {
    int totNodes = 0;
    int totTrees = 0;
    int nHistory = this->orbitals.size();
    for (int i = 0; i < nHistory; i++) {
        int nOrbitals = this->orbitals[i]->size();
        for (int j = 0; j < nOrbitals; j++) {
            totNodes += this->orbitals[i]->getOrbital(j).getNNodes();
        }
        totTrees += nOrbitals;
    }
    for (int i = 0; i < nHistory; i++) {
        int nOrbitals = this->dOrbitals[i]->size();
        for (int j = 0; j < nOrbitals; j++) {
            totNodes += this->dOrbitals[i]->getOrbital(j).getNNodes();
        }
        totTrees += nOrbitals;
    }
    println(0, " Accelerator       " << setw(15) << totTrees << setw(25) << totNodes);
    return totNodes;
}
*/

