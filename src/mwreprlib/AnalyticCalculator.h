#ifndef ANALYTICPROJECTOR_H
#define ANALYTICPROJECTOR_H

#include "TreeCalculator.h"

template<int D>
class AnalyticCalculator : public TreeCalculator<D> {
public:
    AnalyticCalculator(RepresentableFunction<D> &inp_func)
        : func(&inp_func) {
        NOT_IMPLEMENTED_ABORT;
    }
    virtual ~AnalyticCalculator() {
        NOT_IMPLEMENTED_ABORT;
        this->func = 0;
    }

protected:
    RepresentableFunction<D> *func;

    virtual void calcNode(MWNode<D> &node) const;
};

#endif // ANALYTICPROJECTOR_H