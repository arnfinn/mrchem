#ifndef HILBERTPATH_H
#define HILBERTPATH_H

template<int D>
class HilbertPath {
public:
    HilbertPath() { this->path = 0; }
    HilbertPath(const HilbertPath<D> &parent, int cIdx) {
        int hIdx = parent.getHIndex(cIdx);
        this->path = parent.getChildPath(hIdx);
    }
    virtual ~HilbertPath() {}

    short int getPath() const { return this->path; }
    short int getChildPath(int hIdx) const { return this->pTable[this->path][hIdx]; }

    int getZIndex(int hIdx) const { return this->zTable[this->path][hIdx]; }
    int getHIndex(int zIdx) const { return this->hTable[this->path][zIdx]; }

private:
    short int path;
    static constexpr short int pTable[][8];
    static constexpr int zTable[][8];
    static constexpr int hTable[][8];
};

#endif // HILBERTPATH_H