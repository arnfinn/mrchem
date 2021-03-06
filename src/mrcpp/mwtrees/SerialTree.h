/**
*
*
*  \date Jul, 2016
*  \author Peter Wind <peter.wind@uit.no> \n
*  CTCC, University of Tromsø
*
*/

#ifndef SERIALTREE_H_
#define SERIALTREE_H_

template<int D> class MWTree;
template<int D> class MWNode;

template<int D>
class SerialTree {
public:
    SerialTree(MWTree<D> *tree) : tree_p(tree) { }
    virtual ~SerialTree() { }

    MWTree<D>* getTree() { return this->tree_p; }

    virtual void allocRoots(MWTree<D> &tree) = 0;
    virtual void allocChildren(MWNode<D> &parent) = 0;
    virtual void allocGenChildren(MWNode<D> &parent) = 0;

    virtual void deallocNodes(int serialIx) = 0;
    virtual void deallocGenNodes(int serialIx) = 0;

    void S_mwTransform(double* coeff_in, double* coeff_out, bool readOnlyScaling, int stride, bool overwrite = true);
    void S_mwTransformBack(double* coeff_in, double* coeff_out, int stride);

    int nNodes;                 //number of Nodes already defined
    int maxNodesPerChunk;
    int *nodeStackStatus;
    int sizeNodeCoeff;          //size of coeff for one node
    double **coeffStack;

protected:
    int maxNodes;               //max number of nodes that can be defined

    MWTree<D> *tree_p;
};

#endif /* SERIALTREE_H_*/
