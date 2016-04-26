/**
 *
 * \date Jun 5, 2009
 * \author Jonas Juselius <jonas.juselius@uit.no> \n
 *         CTCC, University of Tromsø
 *
 *
 */

#ifndef MWTREE_H_
#define MWTREE_H_

#include <boost/serialization/serialization.hpp>
#include <boost/serialization/utility.hpp>
#include <Eigen/Core>

#include "parallel.h"
#include "mwrepr_declarations.h"

#include "NodeBox.h"
#include "MWNode.h"
#include "MultiResolutionAnalysis.h"

#ifdef OPENMP
#define SET_TREE_LOCK() omp_set_lock(&this->tree_lock)
#define UNSET_TREE_LOCK() omp_unset_lock(&this->tree_lock)
#define TEST_TREE_LOCK() omp_test_lock(&this->tree_lock)
#else
#define SET_TREE_LOCK()
#define UNSET_TREE_LOCK()
#define TEST_TREE_LOCK() false
#endif

template<int D>
class MWTree {
public:
    virtual ~MWTree();
    void setZero();

    double estimateError(bool absPrec);
    double getSquareNorm() const { return this->squareNorm; }

    int getOrder() const { return this->order; }
    int getKp1() const { return this->order + 1; }
    int getKp1_d() const { return this->kp1_d; }
    int getDim() const { return D; }
    int getTDim() const { return this->tDim; }
    int getNNodes(int depth = -1) const;
    int getNEndNodes() const { return this->endNodeTable.size(); }
    int getNAllocGenNodes();
    int getNGenNodes();
    int getRootScale() const { return this->rootBox.getScale(); }
    virtual int getDepth() const { return this->nodesAtDepth.size(); }

    NodeBox<D> &getRootBox() { return this->rootBox; }
    const NodeBox<D> &getRootBox() const { return this->rootBox; }
    const MultiResolutionAnalysis<D> &getMRA() const { return this->MRA; }

    void crop(double thrs = -1.0, bool absPrec = true);
    void mwTransform(int type, bool overwrite = true);

    void setName(const std::string &n) { this->name = n; }
    const std::string &getName() const { return this->name; }

    MWNode<D> *findNode(const NodeIndex<D> &nIdx);
    const MWNode<D> *findNode(const NodeIndex<D> &nIdx) const;

    MWNode<D> &getNode(const NodeIndex<D> &nIdx);
    MWNode<D> &getNodeOrEndNode(const NodeIndex<D> &nIdx);
    const MWNode<D> &getNodeOrEndNode(const NodeIndex<D> &nIdx) const;

    MWNode<D> &getNode(const double *r, int depth = -1);
    MWNode<D> &getNodeOrEndNode(const double *r, int depth = -1);
    const MWNode<D> &getNodeOrEndNode(const double *r, int depth = -1) const;

    MWNode<D> &getEndMWNode(int i) { return *this->endNodeTable[i]; }
    const MWNode<D> &getEndMWNode(int i) const { return *this->endNodeTable[i]; }

    void deleteGenerated();
    void clearGenerated();

    void lockTree() { SET_TREE_LOCK(); }
    void unlockTree() { UNSET_TREE_LOCK(); }
    bool testLock() { return TEST_TREE_LOCK(); }

    int getNThreads() const { return this->nThreads; }
    int getRankId() const { return this->rank; }

    virtual bool saveTree(const std::string &file) { NOT_IMPLEMENTED_ABORT; }
    virtual bool loadTree(const std::string &file) { NOT_IMPLEMENTED_ABORT; }

    int countBranchNodes(int depth = -1);
    int countLeafNodes(int depth = -1);
    int countAllocNodes(int depth = -1);
    int countNodes(int depth = -1);

    friend class MWNode<D>;
    friend class GenNode<D>;
    friend class ProjectedNode<D>;
    friend class TreeBuilder<D>;
    friend class GridCleaner<D>;
    friend class TreeCalculator<D>;
    friend class ProjectionCalculator<D>;
    friend class OperApplicationCalculator<D>;
    friend class OperatorState<D>;

protected:
    // Parameters that are set in construction and should never change
    const int rank;
    const int nThreads;
    const MultiResolutionAnalysis<D> MRA;

    // Static default parameters
    const static int tDim = (1 << D);

    // Constant parameters that are derived internally
    const int order;
    const int kp1_d;

    // Parameters that are dynamic and can be set by user
    std::string name;

    // Tree data
    int nNodes;
    int *nGenNodes;
    int *nAllocGenNodes;
    double squareNorm;
    NodeBox<D> rootBox;            ///< The actual container of nodes
    MWNodeVector endNodeTable;	   ///< Final projected nodes
    std::vector<int> nodesAtDepth;  ///< Node counter

    Eigen::MatrixXd **tmpCoefs;   ///< temp memory
    Eigen::VectorXd **tmpVector;  ///< temp memory
    Eigen::VectorXd **tmpMWCoefs; ///< temp memory

    // Constructors are protected, use TreeBuilders
    MWTree(const MultiResolutionAnalysis<D> &mra);
    MWTree(const MWTree<D> &tree);

    void allocWorkMemory();
    void freeWorkMemory();

    inline Eigen::MatrixXd &getTmpScalingCoefs();
    inline Eigen::VectorXd &getTmpScalingVector();
    inline Eigen::VectorXd &getTmpMWCoefs();

    void calcSquareNorm();
    void clearSquareNorm() { this->squareNorm = -1.0; }

    void mwTransformDown(bool overwrite);
    void mwTransformUp(bool overwrite);

    int getRootIndex(const double *r) const {
        return this->rootBox.getBoxIndex(r);
    }
    int getRootIndex(const NodeIndex<D> &nIdx) const {
        return this->rootBox.getBoxIndex(nIdx);
    }

    void allocNodeCounters();
    void deleteNodeCounters();

    void incrementNodeCount(int scale);
    void decrementNodeCount(int scale);
    void updateGenNodeCounts();
    void incrementGenNodeCount();
    void decrementGenNodeCount();
    void incrementAllocGenNodeCount();
    void decrementAllocGenNodeCount();

    void makeNodeTable(MWNodeVector &nodeTable);
    void makeNodeTable(std::vector<MWNodeVector > &nodeTable);

    MWNodeVector* copyEndNodeTable();
    MWNodeVector* getEndNodeTable() { return &this->endNodeTable; }

    void resetEndNodeTable();
    void clearEndNodeTable() { this->endNodeTable.clear(); }

#ifdef OPENMP
    omp_lock_t tree_lock;
#endif
private:
    friend class boost::serialization::access;

    template<class Archive>
    void save(Archive & ar, const unsigned int version) const {
        NOT_IMPLEMENTED_ABORT;
    }
    template<class Archive>
    void load(Archive & ar, const unsigned int version) {
        NOT_IMPLEMENTED_ABORT;
//        setupFilters(scalingType);
//        setupScalingBasis(scalingType);
//        freeWorkMemory();
//        allocWorkMemory();

//        if (this->isScattered()) {
//            if (node_group.size() < 2) {
//                MSG_WARN("Reading distributed tree in serial. " <<
//                         "Tree is incomplete and unpure.");
//                resetEndNodeTable();
//                return;
//            }
//        }
//        mwTransformUp();
//        resetEndNodeTable();
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER();
};

template<int D>
Eigen::MatrixXd& MWTree<D>::getTmpScalingCoefs() {
    int thread = omp_get_thread_num();
    return *this->tmpCoefs[thread];
}

template<int D>
Eigen::VectorXd& MWTree<D>::getTmpScalingVector() {
    int thread = omp_get_thread_num();
    return *this->tmpVector[thread];
}

template<int D>
Eigen::VectorXd& MWTree<D>::getTmpMWCoefs() {
    int thread = omp_get_thread_num();
    return *this->tmpMWCoefs[thread];
}

#endif /* MWTREE_H_ */
