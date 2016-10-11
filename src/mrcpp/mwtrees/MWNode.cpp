/**
 *  Simple n-dimensional node
 *
 *  Created on: Oct 12, 2009
 *      Author: jonas
 */

#include "MWNode.h"
#include "MWTree.h"
#include "ProjectedNode.h"
#include "MathUtils.h"
#include "QuadratureCache.h"
#include "GenNode.h"
#include "Timer.h"

using namespace std;
using namespace Eigen;

/** MWNode rootnode constructor.
  * Creates an empty rootnode given its tree and node index */
template<int D>
MWNode<D>::MWNode(MWTree<D> &t, const NodeIndex<D> &nIdx)
        : tree(&t),
          parent(0),
          nodeIndex(nIdx),
          hilbertPath(),
          squareNorm(-1.0),
          status(0),
          n_coefs(0),
          coefs(0) {
    clearNorms();
    //    this->coefs = &this->coefvec;
    for (int i = 0; i < getTDim(); i++) {
        this->children[i] = 0;
    }
    setIsLeafNode();
    setIsRootNode();

#ifdef OPENMP
    omp_init_lock(&node_lock);
#endif
}

/** MWNode constructor.
  * Creates an empty node given its parent and child index */
template<int D>
MWNode<D>::MWNode(MWNode<D> &p, int cIdx)
        : tree(p.tree),
          parent(&p),
          nodeIndex(p.getNodeIndex(), cIdx),
          hilbertPath(p.getHilbertPath(), cIdx),
          squareNorm(-1.0),
          status(0),
          n_coefs(0),
          coefs(0) {
    clearNorms();
    //    this->coefs = &this->coefvec;
    for (int i = 0; i < getTDim(); i++) {
        this->children[i] = 0;
    }
    setIsLeafNode();

#ifdef OPENMP
    omp_init_lock(&node_lock);
#endif
}

/** Detatched node */
template<int D>
MWNode<D>::MWNode(const MWNode<D> &n)
        : tree(n.tree),
          parent(0),
          nodeIndex(n.getNodeIndex()),
          hilbertPath(n.getHilbertPath()),
          squareNorm(-1.0),
          status(0),
          n_coefs(0),
          coefs(0) {
  //    this->coefs = &this->coefvec;

    allocCoefs(this->getTDim(), this->getKp1_d());

    if (n.hasCoefs()) {
        setCoefBlock(0, n.n_coefs, n.getCoefs());
	if(this->n_coefs>n.n_coefs)for(int i=n.n_coefs; i<this->n_coefs; i++)this->coefs[i]=0.0; 
    } else {
        zeroCoefBlock(0, this->n_coefs);
    }
    for (int i = 0; i < getTDim(); i++) {
        this->children[i] = 0;
    }
    setIsLeafNode();
    setIsLooseNode();

#ifdef OPENMP
    omp_init_lock(&node_lock);
#endif
}

/** MWNode destructor.
  * Recursive deallocation of a node and all its decendants */
template<int D>
MWNode<D>::~MWNode() {
    if (this->isBranchNode()) {
      deleteChildren();
    }
    if (not this->isLooseNode()) {
      if(!(this->isGenNode()))this->tree->decrementNodeCount(getScale());
      assert(!this->tree->serialTree_p);
    }
    this->freeCoefs();

#ifdef OPENMP
    omp_destroy_lock(&node_lock);
#endif
}

/** Allocate the coefs vector. */
template<int D>
void MWNode<D>::allocCoefs(int n_blocks, int block_size) {
    if (this->isAllocated()) MSG_FATAL("Coefs already allocated");

    if (this->n_coefs != 0) MSG_FATAL("n_coefs should be zero");
    this->n_coefs = n_blocks * block_size;
    if(this->tree->serialTree_p){
 
      /*if (ProjectedNode<D> *node = dynamic_cast<ProjectedNode<D> *>(this)) {
	coefs = this->tree->serialTree_p->allocCoeff(n_blocks,  this);
      } else if (GenNode<D> *node = dynamic_cast<GenNode<D> *>(this)) {
	coefs = this->tree->serialTree_p->allocGenCoeff(n_blocks,  this);
      } else{
	coefs = this->tree->serialTree_p->allocCoeff(n_blocks,  this);
	}*/

      //Only temporary nodes should be allocated here
      coefs = this->tree->serialTree_p->allocLooseCoeff(n_blocks,  this);
      
    }else{
	//Operator Node
	this->coefs = new double[this->n_coefs];
    }


    this->setIsAllocated();
    this->clearHasCoefs();
}

/** Deallocation of coefficients. */
template<int D>
void MWNode<D>::freeCoefs() {
    if (not this->isAllocated()) MSG_FATAL("Coefs not allocated");

    if(this->tree->serialTree_p and this->SNodeIx>=0){
       if(this->SNodeIx<0)cout<<"SNodeIx undefined! "<<this->SNodeIx<<endl;
     if(this->isLooseNode()){
	this->tree->serialTree_p->DeAllocLooseCoeff(this->SNodeIx);
      }else if(this->isGenNode()){
       cout<<"ERROR dealloccoeff done with nodes "<<this->SNodeIx<<endl;
       // 	this->tree->serialTree_p->DeAllocGenCoeff(this->SNodeIx);
     }else{
       cout<<"ERROR  dealloccoeff done with nodes "<<this->SNodeIx<<endl;
       //this->tree->serialTree_p->DeAllocCoeff(this->SNodeIx);
     }
      /*if (ProjectedNode<D> *node = dynamic_cast<ProjectedNode<D> *>(this)) {
	this->tree->serialTree_p->DeAllocCoeff(this->SNodeIx);
      } else if (GenNode<D> *node = dynamic_cast<GenNode<D> *>(this)) {
	this->tree->serialTree_p->DeAllocGenCoeff(this->SNodeIx);
      } else{
	this->tree->serialTree_p->DeAllocCoeff(this->SNodeIx);
	}*/
    }else{
      //Operator node
      if (this->coefs != 0) {
        delete[] this->coefs;
      }
    }
    
    this->coefs = 0;
    this->n_coefs = 0;

    this->clearHasCoefs();
    this->clearIsAllocated();
}

template<int D>
void MWNode<D>::printCoefs() const {
    if (not this->isAllocated()) MSG_FATAL("Node is not allocated");
    //if (not this->hasCoefs()) MSG_FATAL("Node has no coefs");
    println(0, "\nMW coefs");
    int kp1_d = this->getKp1_d();
    for (int i = 0; i < this->n_coefs; i++) {
        println(0, this->coefs[i]);
        if (i%kp1_d == 0) println(0, "\n");
    }
}

template<int D>
void MWNode<D>::getCoefs(Eigen::VectorXd &c) const {
    if (not this->isAllocated()) MSG_FATAL("Node is not allocated");
    if (not this->hasCoefs()) MSG_FATAL("Node has no coefs");
    if (this->n_coefs == 0) MSG_FATAL("ncoefs == 0");

    c = VectorXd::Map(this->coefs, this->n_coefs);
}

template<int D>
void MWNode<D>::zeroCoefs() {
    if (not this->isAllocated()) MSG_FATAL("Coefs not allocated");

    for (int i = 0; i < this->n_coefs; i++) {
        this->coefs[i] = 0.0;
    }
    this->zeroNorms();
    this->setHasCoefs();
}

template<int D>
void MWNode<D>::setCoefBlock(int block, int block_size, const double *c) {
    if (not this->isAllocated()) MSG_FATAL("Coefs not allocated");
    for (int i = 0; i < block_size; i++) {
        this->coefs[block*block_size + i] = c[i];
    }
}

template<int D>
void MWNode<D>::addCoefBlock(int block, int block_size, const double *c) {
    if (not this->isAllocated()) MSG_FATAL("Coefs not allocated");
    for (int i = 0; i < block_size; i++) {
        this->coefs[block*block_size + i] += c[i];
    }
}

template<int D>
void MWNode<D>::zeroCoefBlock(int block, int block_size) {
    if (not this->isAllocated()) MSG_FATAL("Coefs not allocated");
        NOT_IMPLEMENTED_ABORT;
    for (int i = 0; i < block_size; i++) {
        this->coefs[block*block_size + i] = 0.0;
    }
}

template<int D>
void MWNode<D>::giveChildrenCoefs(bool overwrite) {
    assert(this->isBranchNode());
    if (not this->hasCoefs()) MSG_FATAL("No coefficients!");

    if(this->tree->serialTree_p and D!=2){
       //can write directly from parent coeff into children coeff
    //    if(false){

      //coeff of child should be have been allocated already here
      int Children_Stride = this->getMWChild(0).n_coefs;
      bool ReadOnlyScalingCoeff=true;
      if(this->hasWCoefs())ReadOnlyScalingCoeff=false;

      double* coeffin  = this->coefs;
      double* coeffout = this->getMWChild(0).coefs;
      assert(not ( this->getMWChild(0).isGenNode() and  this->getMWChild(0).isLooseNode()));//not implemented

      this->tree->serialTree_p->S_mwTransform(coeffin, coeffout, ReadOnlyScalingCoeff, Children_Stride, overwrite);      

      for (int i = 0; i < this->getTDim(); i++){
	this->getMWChild(i).calcNorms();//should need to compute only scaling norms
	this->getMWChild(i).setHasCoefs();
      }

    }else{

	 MWNode<D> copy(*this);
	 copy.mwTransform(Reconstruction);
	 const double *c = copy.getCoefs();
	 
	 int kp1_d = this->getKp1_d();
	 int nChildren = this->getTDim();
	 for (int i = 0; i < nChildren; i++) {
	   MWNode<D> &child = this->getMWChild(i);
	   if (overwrite) {
	     child.zeroCoefs();
	     child.setCoefBlock(0, kp1_d, &c[i*kp1_d]);
	   } else if (child.hasCoefs()) {
	     child.addCoefBlock(0, kp1_d, &c[i*kp1_d]);
	   } else {
	     MSG_FATAL("Child has no coefs");
	   }
	   child.setHasCoefs();
	   child.calcNorms();
	 }

       }
}

/** Takes the scaling coefficients of the children and stores them consecutively
  * in the  given vector. */
template<int D>
void MWNode<D>::copyCoefsFromChildren() {
    int kp1_d = this->getKp1_d();
    int nChildren = this->getTDim();
    for (int cIdx = 0; cIdx < nChildren; cIdx++) {
        MWNode<D> &child = getMWChild(cIdx);
        if (not child.hasCoefs()) MSG_FATAL("Child has no coefs");
        setCoefBlock(cIdx, kp1_d, child.getCoefs());
    }
}


/** Coefficient-Value transform
  *
  * This routine transforms the scaling coefficients of the node to the
  * function values in the corresponding quadrature roots (of its children).
  * Input parameter = forward: coef->value.
  * Input parameter = backward: value->coef.
  *
  * NOTE: this routine assumes a 0/1 (scaling on children 0 and 1)
  *       representation, in oppose to s/d (scaling and wavelet). */
template<int D>
void MWNode<D>::cvTransform(int operation) {
    const ScalingBasis &sf = this->getMWTree().getMRA().getScalingBasis();
    if (sf.getScalingType() != Interpol) {
        NOT_IMPLEMENTED_ABORT;
    }

    int quadratureOrder = sf.getQuadratureOrder();
    getQuadratureCache(qc);

    double two_scale = pow(2.0, this->getScale() + 1);
    VectorXd modWeights = qc.getWeights(quadratureOrder);
    if (operation == Forward) {
        modWeights = modWeights.array().inverse();
        modWeights *= two_scale;
        modWeights = modWeights.array().sqrt();
    } else if (operation == Backward) {
        modWeights *= 1.0/two_scale;
        modWeights = modWeights.array().sqrt();
    } else {
        MSG_FATAL("Invalid operation");
    }

    int kp1 = this->getKp1();
    int kp1_d = this->getKp1_d();
    int kp1_p[D];
    for (int d = 0; d < D; d++) {
        kp1_p[d] = MathUtils::ipow(kp1, d);
    }

    for (int m = 0; m < this->getTDim(); m++) {
        for (int p = 0; p < D; p++) {
            int n = 0;
            for (int i = 0; i < kp1_p[D - p - 1]; i++) {
                for (int j = 0; j < kp1; j++) {
                    for (int k = 0; k < kp1_p[p]; k++) {
                        this->coefs[m * kp1_d + n] *= modWeights[j];
                        n++;
                    }
                }
            }
        }
    }
}

/** Multiwavelet transform: fast version
  *
  * Application of the filters on one node to pass from a 0/1 (scaling
  * on children 0 and 1) representation to an s/d (scaling and
  * wavelet) representation. Bit manipulation is used in order to
  * determine the correct filters and whether to apply them or just
  * pass to the next couple of indexes. The starting coefficients are
  * preserved until the application is terminated, then they are
  * overwritten. With minor modifications this code can also be used
  * for the inverse mw transform (just use the transpose filters) or
  * for the application of an operator (using A, B, C and T parts of an
  * operator instead of G1, G0, H1, H0). This is the version where the
  * three directions are operated one after the other. Although this
  * is formally faster than the other algorithm, the separation of the
  * three dimensions prevent the possibility to use the norm of the
  * operator in order to discard a priori negligible contributions.

  * Luca Frediani, August 2006
  * C++ version: Jonas Juselius, September 2009 */
template<int D>
void MWNode<D>::orig_mwTransform(int operation) {
    int kp1 = this->getKp1();
    int kp1_dm1 = MathUtils::ipow(kp1, D - 1);
    int kp1_d = this->getKp1_d();
    int nCoefs = this->getTDim()*kp1_d;
    const MWFilter &filter = getMWTree().getMRA().getFilter();
    double overwrite = 0.0;

    double o_vec[nCoefs];
    double *out_vec = o_vec;
    double *in_vec = this->coefs;

    for (int i = 0; i < D; i++) {
        int mask = 1 << i;
        for (int gt = 0; gt < this->getTDim(); gt++) {
            double *out = out_vec + gt * kp1_d;
            for (int ft = 0; ft < this->getTDim(); ft++) {
                /* Operate in direction i only if the bits along other
                 * directions are identical. The bit of the direction we
                 * operate on determines the appropriate filter/operator */
                if ((gt | mask) == (ft | mask)) {
                    double *in = in_vec + ft * kp1_d;
                    int fIdx = 2 * ((gt >> i) & 1) + ((ft >> i) & 1);
                    const MatrixXd &oper = filter.getSubFilter(fIdx, operation);
                    MathUtils::applyFilter(out, in, oper, kp1, kp1_dm1, overwrite);
                    overwrite = 1.0;
                }
            }
            overwrite = 0.0;
        }
        double *tmp = in_vec;
        in_vec = out_vec;
        out_vec = tmp;
    }
    if (IS_ODD(D)) {
        for (int i = 0; i < nCoefs; i++) {
            this->coefs[i] = in_vec[i];
        }
    }
}

/** Multiwavelet transform: fast version
  *
  * Application of the filters on one node to pass from a 0/1 (scaling
  * on children 0 and 1) representation to an s/d (scaling and
  * wavelet) representation. Bit manipulation is used in order to
  * determine the correct filters and whether to apply them or just
  * pass to the next couple of indexes. The starting coefficients are
  * preserved until the application is terminated, then they are
  * overwritten. With minor modifications this code can also be used
  * for the inverse mw transform (just use the transpose filters) or
  * for the application of an operator (using A, B, C and T parts of an
  * operator instead of G1, G0, H1, H0). This is the version where the
  * three directions are operated one after the other. Although this
  * is formally faster than the other algorithm, the separation of the
  * three dimensions prevent the possibility to use the norm of the
  * operator in order to discard a priori negligible contributions.

  * Luca Frediani, August 2006
  * C++ version: Jonas Juselius, September 2009 */
template<int D>
void MWNode<D>::mwTransform(int operation) {
    int kp1 = this->getKp1();
    int kp1_dm1 = MathUtils::ipow(kp1, D - 1);
    int kp1_d = this->getKp1_d();
    int nCoefs = this->getTDim()*kp1_d;
    const MWFilter &filter = getMWTree().getMRA().getFilter();
    double overwrite = 0.0;

    double o_vec[nCoefs];
    double *out_vec = o_vec;
    double *in_vec = this->coefs;

    for (int i = 0; i < D; i++) {
        int mask = 1 << i;
        for (int gt = 0; gt < this->getTDim(); gt++) {
            double *out = out_vec + gt * kp1_d;
            for (int ft = 0; ft < this->getTDim(); ft++) {
                /* Operate in direction i only if the bits along other
                 * directions are identical. The bit of the direction we
                 * operate on determines the appropriate filter/operator */
                if ((gt | mask) == (ft | mask)) {
                    double *in = in_vec + ft * kp1_d;
                    int fIdx = 2 * ((gt >> i) & 1) + ((ft >> i) & 1);
                    const MatrixXd &oper = filter.getSubFilter(fIdx, operation);
                    MathUtils::applyFilter(out, in, oper, kp1, kp1_dm1, overwrite);
                    overwrite = 1.0;
                }
            }
            overwrite = 0.0;
        }
        double *tmp = in_vec;
        in_vec = out_vec;
        out_vec = tmp;
    }
    if (IS_ODD(D)) {
        for (int i = 0; i < nCoefs; i++) {
            this->coefs[i] = in_vec[i];
        }
    }
}

/** Set all norms to Undefined. */
template<int D>
void MWNode<D>::clearNorms() {
    this->squareNorm = -1.0;
    for (int i = 0; i < this->getTDim(); i++) {
        this->componentNorms[i] = -1.0;
    }
}

/** Set all norms to zero. */
template<int D>
void MWNode<D>::zeroNorms() {
    this->squareNorm = 0.0;
    for (int i = 0; i < this->getTDim(); i++) {
        this->componentNorms[i] = 0.0;
    }
}

/** Calculate and store square norm and component norms, if allocated. */
template<int D>
void MWNode<D>::calcNorms() {
    this->squareNorm = 0.0;
    for (int i = 0; i < this->getTDim(); i++) {
        double norm_i = calcComponentNorm(i);
        this->componentNorms[i] = norm_i;
        this->squareNorm += norm_i*norm_i;
    }
}

/** Calculate and return the squared scaling norm. */
template<int D>
double MWNode<D>::getScalingNorm() const {
    double sNorm = this->getComponentNorm(0);
    if (sNorm >= 0.0) {
        return sNorm*sNorm;
    } else {
        return -1.0;
    }
}

/** Calculate and return the squared wavelet norm. */
template<int D>
double MWNode<D>::getWaveletNorm() const {
    double wNorm = 0.0;
    for (int i = 1; i < this->getTDim(); i++) {
        double norm_i = this->getComponentNorm(i);
        if (norm_i >= 0.0) {
            wNorm += norm_i*norm_i;
        } else {
            wNorm = -1.0;
        }
    }
    return wNorm;
}

/** Calculate the norm of one component (NOT the squared norm!). */
template<int D>
double MWNode<D>::calcComponentNorm(int i) const {
  assert(i==0 or (not this->isGenNode()));//GenNodes have no wavelets coefficients
    assert(this->isAllocated());
    assert(this->hasCoefs());

    const double *c = this->getCoefs();
    int size = this->getKp1_d();
    int start = i*size;

    double sq_norm = 0.0;
#ifdef HAVE_BLAS
    sq_norm = cblas_ddot(size, &c[start], 1, &c[start], 1);
#else
    for (int i = start; i < start+size; i++) {
        sq_norm += c[i]*c[i];
    }
#endif
    return sqrt(sq_norm);
}

template<int D>
double MWNode<D>::estimateError(bool absPrec) {
    NOT_IMPLEMENTED_ABORT;
    //    if (this->isForeign()) {
    //        return 0.0;
    //    }
    //    if (this->isCommon() and this->tree->getRankId() != 0) {
    //        return 0.0;
    //    }
    //    double tNorm = 1.0;
    //    if (not absPrec) {
    //        tNorm = sqrt(getMWTree().getSquareNorm());
    //    }

    //    int n = this->getScale();
    //    double expo = (1.0 * (n + 1));
    //    double scaleFactor = max(2.0* MachinePrec, pow(2.0, -expo));
    //    double wNorm = this->calcWaveletNorm();
    //    double error = scaleFactor * wNorm / tNorm;
    //    return error*error;
}

/** Update the coefficients of the node by a mw transform of the scaling
  * coefficients of the children. Option to overwrite or add up existing
  * coefficients. */
template<int D>
void MWNode<D>::reCompress(bool overwrite) {
    if ((not this->isGenNode()) and this->isBranchNode()) {
        if (not this->isAllocated()) MSG_FATAL("Coefs not allocated");
        if (overwrite) {
	  if(this->tree->serialTree_p and D==3){
	    //can write directly from children coeff into parent coeff
	    int Children_Stride = this->getMWChild(0).n_coefs;
	    double* coeffin  = this->getMWChild(0).coefs;
	    double* coeffout = this->coefs;
 
	    assert(coeffin+((1<<D)-1)*Children_Stride == this->getMWChild((1<<D)-1).coefs);
 
	    this->tree->serialTree_p->S_mwTransformBack(coeffin, coeffout, Children_Stride);
	  }else{
            copyCoefsFromChildren();
            mwTransform(Compression);
	  }
        } else {
            // Check optimization
            NOT_IMPLEMENTED_ABORT;
            //MatrixXd tmp = getCoefs();
            //copyCoefsFromChildren(*this->coefs);
            //mwTransform(Compression);
            //getCoefs() += tmp;
        }
        this->setHasCoefs();
        this->calcNorms();
    }
}

/** Recurse down until an EndNode is found, and then crop children with
  * too high precision. */
template<int D>
bool MWNode<D>::crop(double prec, NodeIndexSet *cropIdx) {
    NOT_IMPLEMENTED_ABORT;
    //    if (this->isEndNode()) {
    //        return true;
    //    } else {
    //        for (int i = 0; i < this->tDim; i++) {
    //            MWNode<D> &child = *this->children[i];
    //            if (child.cropChildren(prec, cropIdx)) {
    //                if (not this->isForeign()) {
    //                    if (this->splitCheck(prec) == false) {
    //                        if (cropIdx != 0) {
    //                            cropIdx->insert(&this->getNodeIndex());
    //                        } else {
    //                            this->deleteChildren();
    //                        }
    //                        return true;
    //                    }
    //                }
    //            }
    //        }
    //    }
    //    return false;
}

template<int D>
void MWNode<D>::createChildren() {
    if (this->isBranchNode()) MSG_FATAL("Node already has children");
    int nChildren = this->getTDim();

    //NB: serial tree MUST generate all children consecutively
    //all children must be generated at once if several threads are active
    if (this->tree->serialTree_p){
      int NodeIx;
      double *coefs_p;
      //reserve place for nChildren
      ProjectedNode<D>* ProjNode_p = this->tree->serialTree_p->allocNodes(nChildren, &NodeIx, &coefs_p);
      MWNode<D> *child;
      for (int cIdx = 0; cIdx < nChildren; cIdx++) {

 	  *(char**)(ProjNode_p)=this->tree->serialTree_p->cvptr_ProjectedNode;
	  ProjNode_p->SNodeIx = NodeIx;
	  ProjNode_p->tree = this->tree;
	  ProjNode_p->parent = this;
	  ProjNode_p->parentSNodeIx = this->SNodeIx;
	  ProjNode_p->nodeIndex = NodeIndex<D>(this->getNodeIndex(),cIdx);
	  ProjNode_p->hilbertPath = HilbertPath<D>(this->getHilbertPath(), cIdx);
	  ProjNode_p->squareNorm = -1.0;
	  ProjNode_p->status = 0;

	  ProjNode_p->zeroNorms();
	  for (int i = 0; i < ProjNode_p->getTDim(); i++) {
	    ProjNode_p->children[i] = 0;
	    ProjNode_p->childSNodeIx[i] = -1;
	  }
	  ProjNode_p->setIsLeafNode();
	  ProjNode_p->coefs = coefs_p;
	  ProjNode_p->n_coefs = this->tree->serialTree_p->sizeNodeCoeff;
	  ProjNode_p->setIsAllocated();
	  ProjNode_p->setHasCoefs();
	  ProjNode_p->setIsEndNode();
	  ProjNode_p->setHasWCoefs();

	  //	  ProjNode_p->zeroCoefs();//SHOULD BE REMOVED!

	  ProjNode_p->tree->incrementNodeCount(ProjNode_p->getScale());
	  
	  this->children[cIdx] = ProjNode_p;
	  this->childSNodeIx[cIdx] = ProjNode_p->SNodeIx;

#ifdef OPENMP
	  omp_init_lock(&ProjNode_p->node_lock);
#endif

	ProjNode_p++;
	NodeIx++;
        coefs_p += this->tree->serialTree_p->sizeNodeCoeff;	
	
      }
    }else{
      for (int cIdx = 0; cIdx < getTDim(); cIdx++) {
        createChild(cIdx);
      }
    }
    this->setIsBranchNode();
}

/** Recursive deallocation of children and all their descendants.
  * Leaves node as LeafNode and children[] as null pointer. */
template<int D> void MWNode<D>::deleteChildren() {
     for (int cIdx = 0; cIdx < getTDim(); cIdx++) {
       if (this->children[cIdx] != 0) {
	 if(this->tree->serialTree_p){
	   MWNode<D> *node = this->children[cIdx];
	   assert(!node->isLooseNode());
	   if(node->isBranchNode())node->deleteChildren();
	   if(node->isGenNode()){
	     this->tree->decrementGenNodeCount();
	     this->tree->decrementAllocGenNodeCount();
	     this->tree->serialTree_p->DeAllocGenNodes(node->SNodeIx);
	     //	     this->tree->serialTree_p->DeAllocGenCoeff(node->SNodeIx);
	   }else{
	     this->tree->serialTree_p->DeAllocNodes(node->SNodeIx);
	     //this->tree->serialTree_p->DeAllocCoeff(node->SNodeIx);
	     this->tree->decrementNodeCount(node->getScale());
	   }
	 }else{
	   //ProjectedNode<D> *Opnode = static_cast<ProjectedNode<D> *>(this->children[cIdx]);
	   // delete Opnode;
	   delete this->children[cIdx];
	 }
	 this->children[cIdx] = 0;
       }
     } 
     this->setIsLeafNode();
}

template<int D>
void MWNode<D>::genChildren() {
    if (this->isBranchNode()) MSG_FATAL("Node already has children");
    int nChildren = this->getTDim();

    //NB: serial tree MUST generate all children consecutively
    //all children must be generated at once if several threads are active
    if (this->tree->serialTree_p){
      int NodeIx;
      double *coefs_p;
      //reserve place for nChildren
      GenNode<D>* GenNode_p = this->tree->serialTree_p->allocGenNodes(nChildren, &NodeIx, &coefs_p);
      MWNode<D> *child;
      for (int cIdx = 0; cIdx < nChildren; cIdx++) {
	//if(true){
	if(false){
	  if (ProjectedNode<D> *node = dynamic_cast<ProjectedNode<D> *>(this)) {
	    child = new (GenNode_p)GenNode<D>(*node, cIdx);
	  } else if (GenNode<D> *node = dynamic_cast<GenNode<D> *>(this)){
	    child = new (GenNode_p)GenNode<D>(*node, cIdx);
	  }else{
	    MSG_FATAL("genChildren error");
	  }
	  this->children[cIdx] = child;
	  //Noderank is written in allocGenNodes already!
	  //child->SNodeIx = NodeIx;
	}else{

 	  *(char**)(GenNode_p)=this->tree->serialTree_p->cvptr_GenNode;
	  GenNode_p->SNodeIx = NodeIx;
	  GenNode_p->tree = this->tree;
	  GenNode_p->parent = this;
	  GenNode_p->parentSNodeIx = this->SNodeIx;
	  GenNode_p->nodeIndex = NodeIndex<D>(this->getNodeIndex(),cIdx);
	  GenNode_p->hilbertPath = HilbertPath<D>(this->getHilbertPath(), cIdx);
	  GenNode_p->squareNorm = -1.0;
	  GenNode_p->status = 0;

	  GenNode_p->zeroNorms();
	  for (int i = 0; i < GenNode_p->getTDim(); i++) {
	    GenNode_p->children[i] = 0;
	    GenNode_p->childSNodeIx[i] = -1;
	  }
	  GenNode_p->setIsLeafNode();
	  GenNode_p->coefs = coefs_p;
	  GenNode_p->n_coefs = this->tree->serialTree_p->sizeGenNodeCoeff;
	  GenNode_p->setIsAllocated();
	  GenNode_p->setHasCoefs();

	  //	  GenNode_p->zeroCoefs();//SHOULD BE REMOVED!

	  GenNode_p->tree->incrementGenNodeCount();
	  GenNode_p->tree->incrementAllocGenNodeCount();
	  GenNode_p->setIsGenNode();
	  //GenNode_p->clearHasWCoefs();//default until known
	  
	  this->children[cIdx] = GenNode_p;
	  this->childSNodeIx[cIdx] = GenNode_p->SNodeIx;

#ifdef OPENMP
	  omp_init_lock(&GenNode_p->node_lock);
#endif

	}

	GenNode_p++;
	NodeIx++;
        coefs_p += this->tree->serialTree_p->sizeGenNodeCoeff;	
	
      }
    }else{
      for (int cIdx = 0; cIdx < nChildren; cIdx++) {
	genChild(cIdx);
      }
    }
    this->setIsBranchNode();
}

/** Clear coefficients of generated nodes.
  *
  * The node structure is kept, only the coefficients are cleared. */
template<int D>
void MWNode<D>::clearGenerated() {
    if (this->isBranchNode()) {
        assert(this->children != 0);
        for (int cIdx = 0; cIdx < this->getTDim(); cIdx++) {
            if (this->children[cIdx] != 0) {
                this->getMWChild(cIdx).clearGenerated();
            }
        }
    }
}

template<int D>
void MWNode<D>::deleteGenerated() {
    if (this->isBranchNode()) {      
        if (this->isEndNode()) {
            this->deleteChildren();
        } else {
            for (int cIdx = 0; cIdx < getTDim(); cIdx++) {
                assert(this->children[cIdx] != 0);
                this->getMWChild(cIdx).deleteGenerated();
            }
        }
    }
}

template<int D>
void MWNode<D>::getCenter(double *r) const {
    NOT_IMPLEMENTED_ABORT;
    //    assert(r != 0);
    //    double sFac = pow(2.0, -getScale());
    //    for (int d = 0; d < D; d++) {
    //        double l = (double) getTranslation()[d];
    //        r[d] = sFac*(l + 0.5);
    //    }
}

template<int D>
void MWNode<D>::getBounds(double *lb, double *ub) const {
    int n = getScale();
    double p = pow(2.0, -n);
    const int *l = getTranslation();
    for (int i = 0; i < D; i++) {
        lb[i] = p * l[i];
        ub[i] = p * (l[i] + 1);
    }
}

/** Routine to find the path along the tree.
  *
  * Given the translation indices at the final scale, computes the child m
  * to be followed at the current scale in oder to get to the requested
  * node at the final scale. The result is the index of the child needed.
  * The index is obtained by bit manipulation of of the translation indices. */
template<int D>
int MWNode<D>::getChildIndex(const NodeIndex<D> &nIdx) const {
    assert(isAncestor(nIdx));
    int cIdx = 0;
    int diffScale = nIdx.getScale() - getScale() - 1;
    assert(diffScale >= 0);
    for (int d = 0; d < D; d++) {
        int bit = (nIdx.getTranslation()[d] >> (diffScale)) & 1;
        cIdx = cIdx + (bit << d);
    }
    assert(cIdx >= 0);
    assert(cIdx < getTDim());
    return cIdx;
}

/** Routine to find the path along the tree.
  *
  * Given a point in space, determines which child should be followed
  * to get to the corresponding terminal node. */
template<int D>
int MWNode<D>::getChildIndex(const double *r) const {
    assert(hasCoord(r));
    int cIdx = 0;
    double sFac = pow(2.0, -getScale());
    const int *l = getTranslation();
    for (int d = 0; d < D; d++) {
        if (r[d] > sFac*(l[d] + 0.5)) {
            cIdx = cIdx + (1 << d);
        }
    }
    assert(cIdx >= 0);
    assert(cIdx < getTDim());
    return cIdx;
}

/** Const version of node retriever that NEVER generates.
  *
  * Recursive routine to find and return the node with a given NodeIndex.
  * This routine returns the appropriate ProjectedNode, or a NULL pointer if
  * the node does not exist, or if it is a GenNode. Recursion starts at at this
  * node and ASSUMES the requested node is in fact decending from this node. */
template<int D>
const MWNode<D> *MWNode<D>::retrieveNodeNoGen(const NodeIndex<D> &idx) const {
    if (getScale() == idx.getScale()) { // we're done
        assert(getNodeIndex() == idx);
        return this;
    }
    assert(this->isAncestor(idx));
    if (this->isEndNode()) { // don't return GenNodes
        return 0;
    }
    int cIdx = getChildIndex(idx);
    assert(this->children[cIdx] != 0);
    return this->children[cIdx]->retrieveNodeNoGen(idx);
}

/** Node retriever that NEVER generates.
  *
  * Recursive routine to find and return the node with a given NodeIndex.
  * This routine returns the appropriate ProjectedNode, or a NULL pointer if
  * the node does not exist, or if it is a GenNode. Recursion starts at at this
  * node and ASSUMES the requested node is in fact decending from this node. */
template<int D>
MWNode<D> *MWNode<D>::retrieveNodeNoGen(const NodeIndex<D> &idx) {
    if (getScale() == idx.getScale()) { // we're done
        assert(getNodeIndex() == idx);
        return this;
    }
    assert(this->isAncestor(idx));
    if (this->isEndNode()) { // don't return GenNodes
        return 0;
    }
    int cIdx = getChildIndex(idx);
    assert(this->children[cIdx] != 0);
    return this->children[cIdx]->retrieveNodeNoGen(idx);
}

template<int D>
const MWNode<D> *MWNode<D>::retrieveNodeOrEndNode(const double *r, int depth) const {
    if (getDepth() == depth or this->isEndNode()) {
        return this;
    }
    int cIdx = getChildIndex(r);
    assert(this->children[cIdx] != 0);
    return this->children[cIdx]->retrieveNodeOrEndNode(r, depth);
}

/** Node retriever that return requested ProjectedNode or EndNode.
  *
  * Recursive routine to find and return the node with a given NodeIndex.
  * This routine returns the appropriate ProjectedNode, or the EndNode on the
  * path to the requested node, and will never create or return GenNodes.
  * Recursion starts at at this node and ASSUMES the requested node is in fact
  * decending from this node. */
template<int D>
MWNode<D> *MWNode<D>::retrieveNodeOrEndNode(const double *r, int depth) {
    if (getDepth() == depth or this->isEndNode()) {
        return this;
    }
    int cIdx = getChildIndex(r);
    assert(this->children[cIdx] != 0);
    return this->children[cIdx]->retrieveNodeOrEndNode(r, depth);
}

template<int D>
const MWNode<D> *MWNode<D>::retrieveNodeOrEndNode(const NodeIndex<D> &idx) const {
    if (getScale() == idx.getScale()) { // we're done
        assert(getNodeIndex() == idx);
        return this;
    }
    assert(isAncestor(idx));
    // We should in principle lock before read, but it makes things slower,
    // and the EndNode status does not change (normally ;)
    if (isEndNode()) {
        return this;
    }
    int cIdx = getChildIndex(idx);
    assert(children[cIdx] != 0);
    return this->children[cIdx]->retrieveNodeOrEndNode(idx);
}

template<int D>
MWNode<D> *MWNode<D>::retrieveNodeOrEndNode(const NodeIndex<D> &idx) {
    if (getScale() == idx.getScale()) { // we're done
        assert(getNodeIndex() == idx);
        return this;
    }
    assert(isAncestor(idx));
    // We should in principle lock before read, but it makes things slower,
    // and the EndNode status does not change (normally ;)
    if (isEndNode()) {
        return this;
    }
    int cIdx = getChildIndex(idx);
    assert(children[cIdx] != 0);
    return this->children[cIdx]->retrieveNodeOrEndNode(idx);
}

/** Node retriever that ALWAYS returns the requested node.
  *
  * Recursive routine to find and return the node with a given NodeIndex.
  * This routine always returns the appropriate node, and will generate nodes
  * that does not exist. Recursion starts at this node and ASSUMES the
  * requested node is in fact decending from this node. */
template<int D>
MWNode<D> *MWNode<D>::retrieveNode(const double *r, int depth) {
    if (depth < 0) MSG_FATAL("Invalid argument");

    if (getDepth() == depth) {
        return this;
    }
    assert(hasCoord(r));
    // If we have reached an endNode, lock if necessary, and start generating
    // NB! retrieveNode() for GenNodes behave a bit differently.
    SET_NODE_LOCK();
    if (this->isLeafNode()) {
        genChildren();
        giveChildrenCoefs();
   }
    UNSET_NODE_LOCK();
    int cIdx = getChildIndex(r);
    assert(this->children[cIdx] != 0);
    return this->children[cIdx]->retrieveNode(r, depth);
}

/** Node retriever that ALWAYS returns the requested node, possibly without coefs.
  *
  * Recursive routine to find and return the node with a given NodeIndex. This
  * routine always returns the appropriate node, and will generate nodes that
  * does not exist. Recursion starts at this node and ASSUMES the requested
  * node is in fact decending from this node. */
template<int D>
MWNode<D> *MWNode<D>::retrieveNode(const NodeIndex<D> &idx) {
    if (getScale() == idx.getScale()) { // we're done
        assert(getNodeIndex() == idx);
        return this;
    }
    assert(isAncestor(idx));
    SET_NODE_LOCK();
    if (isLeafNode()) {
      genChildren();
      giveChildrenCoefs();
    }
    UNSET_NODE_LOCK();
    int cIdx = getChildIndex(idx);

    assert(this->children[cIdx] != 0);
    return this->children[cIdx]->retrieveNode(idx);
}

/** Test if a given coordinate is within the boundaries of the node. */
template<int D>
bool MWNode<D>::hasCoord(const double *r) const {
    double sFac = pow(2.0, -getScale());
    const int *l = getTranslation();
    //    println(1, "[" << r[0] << "," << r[1] << "," << r[2] << "]");
    //    println(1, "[" << l[0] << "," << l[1] << "," << l[2] << "]");
    //    println(1, *this);
    for (int d = 0; d < D; d++) {
        if (r[d] < sFac*l[d] or r[d] > sFac*(l[d] + 1)) {
            //            println(1, "false");
            return false;
        }
    }
    //    println(1, "true");
    return true;
}

/** Testing if nodes are compatible wrt NodeIndex and Tree (order, rootScale,
  * relPrec, etc). */
template<int D>
bool MWNode<D>::isCompatible(const MWNode<D> &node) {
    NOT_IMPLEMENTED_ABORT;
    //    if (nodeIndex != node.nodeIndex) {
    //        println(0, "nodeIndex mismatch" << std::endl);
    //        return false;
    //    }
    //    if (not this->tree->checkCompatible(*node.tree)) {
    //        println(0, "tree type mismatch" << std::endl);
    //        return false;
    //    }
    //    return true;
}

/** Test if the node is decending from a given NodeIndex, that is, if they have
  * overlapping support. */
template<int D>
bool MWNode<D>::isAncestor(const NodeIndex<D> &idx) const {
    int relScale = idx.getScale() - getScale();
    if (relScale < 0) {
        return false;
    }
    const int *l = getTranslation();
    for (int d = 0; d < D; d++) {
        int reqTransl = idx.getTranslation()[d] >> relScale;
        if (l[d] != reqTransl) {
            return false;
        }
    }
    return true;
}

template<int D>
bool MWNode<D>::isDecendant(const NodeIndex<D> &idx) const {
    NOT_IMPLEMENTED_ABORT;
}

template class MWNode<1>;
template class MWNode<2>;
template class MWNode<3>;
