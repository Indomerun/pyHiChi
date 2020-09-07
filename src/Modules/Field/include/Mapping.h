#pragma once
#include "Vectors.h"
#include "Constants.h"
#include "Enums.h"
#include <functional>

namespace pfc {

    class Mapping {

    public:

        virtual FP3 getDirectCoords(const FP3& coords, FP time = 0.0, bool* status = 0) {
            setOkStatus(status);
            return coords;
        }

        virtual FP3 getInverseCoords(const FP3& coords, FP time = 0.0, bool* status = 0) {
            setOkStatus(status);
            return coords;
        }

        virtual Mapping* createInstance() = 0;

    protected:

        void setFailStatus(bool* status) {
            if (status) *status = false;
        }

        void setOkStatus(bool* status) {
            if (status) *status = true;
        }

    };


    class IdentityMapping : public Mapping {

    public:

        // create identity mapping on a segment [a,b)
        IdentityMapping(FP3 a, FP3 b) : a(a), b(b) {}

        FP3 getDirectCoords(const FP3& coords, FP time = 0.0, bool* status = 0) override {
            if (status) *status = (coords >= a && coords < b) ? true : false;
            return coords;
        }

        FP3 getInverseCoords(const FP3& coords, FP time = 0.0, bool* status = 0) override {
            if (status) *status = (coords >= a && coords < b) ? true : false;
            return coords;
        }

        Mapping* createInstance() override {
            return new IdentityMapping(*this);
        }

        FP3 a, b;

    };


    class PeriodicalMapping : public Mapping {

    public:

        // create periodical mapping: axis = ...[cMin, cMax)[cMin, cMax)[cMin, cMax)...
        PeriodicalMapping(Coordinate axis, FP cMin, FP cMax) :
            axis(axis), cMin(cMin), cMax(cMax), D(cMax-cMin) {}

        FP3 getDirectCoords(const FP3& coords, FP time = 0.0, bool* status = 0) override {
            if (status) *status = (coords[axis] >= cMin && coords[axis] < cMax) ? true : false;
            return coords;
        }

        FP3 getInverseCoords(const FP3& coords, FP time = 0.0, bool* status = 0) override {
            setOkStatus(status);
            FP3 inverseCoords = coords;
            double tmp;
            FP frac = std::modf((coords[axis] - cMin) / D, &tmp);
            inverseCoords[axis] = cMin + (frac >= 0 ? frac : (1 + frac)) * D;
            return inverseCoords;
        }

        Mapping* createInstance() override {
            return new PeriodicalMapping(*this);
        }

        FP cMin, cMax, D;
        Coordinate axis = Coordinate::x;

    };


    class RotationMapping : public Mapping {

    public:

        // rotation occurs around the axis according to the rule of the right hand
        RotationMapping(Coordinate axis, double angle): axis(axis), angle(angle) {
            Coordinate axis1 = Coordinate(((int)axis + 1) % 3),
                axis2 = Coordinate(((int)axis + 2) % 3);
            rotationMatrix[axis1][axis1] = cos(angle);
            rotationMatrix[axis1][axis2] = -sin(angle);
            rotationMatrix[axis2][axis1] = sin(angle);
            rotationMatrix[axis2][axis2] = cos(angle);
            rotationMatrix[axis][axis] = 1.0;
        }

        FP3 getDirectCoords(const FP3& coords, FP time = 0.0, bool* status = 0) override {
            setOkStatus(status);
            FP3 directCoords;
            directCoords.x =
                rotationMatrix[Coordinate::x][Coordinate::x] * coords.x +
                rotationMatrix[Coordinate::x][Coordinate::y] * coords.y +
                rotationMatrix[Coordinate::x][Coordinate::z] * coords.z;
            directCoords.y =
                rotationMatrix[Coordinate::y][Coordinate::x] * coords.x +
                rotationMatrix[Coordinate::y][Coordinate::y] * coords.y +
                rotationMatrix[Coordinate::y][Coordinate::z] * coords.z;
            directCoords.z =
                rotationMatrix[Coordinate::z][Coordinate::x] * coords.x +
                rotationMatrix[Coordinate::z][Coordinate::y] * coords.y +
                rotationMatrix[Coordinate::z][Coordinate::z] * coords.z;
            return directCoords;
        }

        FP3 getInverseCoords(const FP3& coords, FP time = 0.0, bool* status = 0) override {
            setOkStatus(status);
            FP3 inverseCoords;
            inverseCoords.x =
                rotationMatrix[Coordinate::x][Coordinate::x] * coords.x +
                rotationMatrix[Coordinate::y][Coordinate::x] * coords.y +
                rotationMatrix[Coordinate::z][Coordinate::x] * coords.z;
            inverseCoords.y =
                rotationMatrix[Coordinate::x][Coordinate::y] * coords.x +
                rotationMatrix[Coordinate::y][Coordinate::y] * coords.y +
                rotationMatrix[Coordinate::z][Coordinate::y] * coords.z;
            inverseCoords.z =
                rotationMatrix[Coordinate::x][Coordinate::z] * coords.x +
                rotationMatrix[Coordinate::y][Coordinate::z] * coords.y +
                rotationMatrix[Coordinate::z][Coordinate::z] * coords.z;
            return inverseCoords;
        }

        Mapping* createInstance() override {
            return new RotationMapping(*this);
        }

        Coordinate axis;
        FP angle;  // in radians
        // mapping from inverse to direct coords
        FP rotationMatrix[3][3] = { {0.0,0.0,0.0},{0.0,0.0,0.0},{0.0,0.0,0.0} };

    };


    class ShiftMapping : public Mapping {

    public:

        ShiftMapping(const FP3& shift) : shift(shift) {}

        FP3 getDirectCoords(const FP3& coords, FP time = 0.0, bool* status = 0) override {
            setOkStatus(status);
            return coords + shift;
        }

        FP3 getInverseCoords(const FP3& coords, FP time = 0.0, bool* status = 0) override {
            setOkStatus(status);
            return coords - shift;
        }

        Mapping* createInstance() override {
            return new ShiftMapping(*this);
        }

        FP3 shift;

    };


    class ScaleMapping : public Mapping {

    public:

        ScaleMapping(Coordinate axis, FP coef) : axis(axis), coef(coef) {}

        FP3 getDirectCoords(const FP3& coords, FP time = 0.0, bool* status = 0) override {
            setOkStatus(status);
            FP3 directCoords = coords;
            directCoords[axis] *= coef;
            return directCoords;
        }

        FP3 getInverseCoords(const FP3& coords, FP time = 0.0, bool* status = 0) override {
            setOkStatus(status);
            FP3 inverseCoords = coords;
            inverseCoords[axis] /= coef;
            return inverseCoords;
        }

        Mapping* createInstance() override {
            return new ScaleMapping(*this);
        }

        FP coef;
        Coordinate axis;

    };


    class TightFocusingMapping : public Mapping {

    public:

        TightFocusingMapping(FP R0, FP L, FP D, Coordinate axis = Coordinate::x) :
            Rmax(R0 + 0.5*L), ifCut(true),
            periodicalMapping(axis, -R0 - D + 0.5*L, -R0 + 0.5*L)
        {}

        void setIfCut(bool ifCut = true) {
            this->ifCut = ifCut;
        }

        FP3 getDirectCoords(const FP3& coords, FP time = 0.0, bool* status = 0) override {
            FP ct = constants::c*time;
            FP r = coords.norm();

            FP3 directCoords = coords;
            if (ifCut) this->setFailStatus(status);
            else setOkStatus(status);

            if (coords[periodicalMapping.axis] < periodicalMapping.cMin + ct ||
                coords[periodicalMapping.axis] >= periodicalMapping.cMax + ct)
                return coords;

            int nPeriods = 0;
            FP shift = 0;

            if (periodicalMapping.cMax + ct < 0) {
                nPeriods = int(-(periodicalMapping.cMin + ct) / periodicalMapping.D) + 1;  // ����� ����� ������
                shift = periodicalMapping.D;
            }
            else if (periodicalMapping.cMin + ct > 0) {
                nPeriods = int((periodicalMapping.cMax + ct) / periodicalMapping.D) + 1;  // ����� ����� ������
                shift = -periodicalMapping.D;
            }
            else nPeriods = 1;

            FP3 coordsShift = coords;

            for (int i = 0; i < nPeriods; i++) {

                coordsShift[periodicalMapping.axis] += shift;

                if (ifInArea(coordsShift, time)) {
                    directCoords = coordsShift;
                    setOkStatus(status);
                    break;
                }
            }

            return directCoords;
        }

        FP3 getInverseCoords(const FP3& coords, FP time = 0.0, bool* status = 0) override {

            setOkStatus(status);

            if (ifCut && !ifInArea(coords, time)) {
                setFailStatus(status);
            }

            return periodicalMapping.getInverseCoords(coords);

        }

        FP getMinCoord() const { return periodicalMapping.cMin; }
        FP getMaxCoord() const { return periodicalMapping.cMax; }

        bool ifInArea(const FP3& coords, FP time) {
            FP ct = constants::c*time;
            FP r = coords.norm();

            if (periodicalMapping.cMax + ct < 0) {
                if ((r >= Rmax - ct) || (r < -periodicalMapping.cMax - ct) ||
                    (coords[periodicalMapping.axis] > 0))
                    return false;
            }
            else if ((periodicalMapping.cMax + ct >= 0) && (-Rmax + ct <= 0))
            {
                if (coords[periodicalMapping.axis] < 0 && r > periodicalMapping.cMax + Rmax)
                    return false;
                if (coords[periodicalMapping.axis] >= 0 && r > periodicalMapping.cMax + ct)
                    return false;
            }
            else if (-Rmax + ct > 0)
            {
                if ((r <= -Rmax + ct) || (r > periodicalMapping.cMax + ct) ||
                    (coords[periodicalMapping.axis] < 0))
                    return false;
            }

            return true;
        }

        Mapping* createInstance() override {
            return new TightFocusingMapping(*this);
        }

        FP Rmax;
        bool ifCut = true;
        PeriodicalMapping periodicalMapping;

    };

}