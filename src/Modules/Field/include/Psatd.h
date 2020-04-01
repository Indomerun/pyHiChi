#pragma once
#include "Constants.h"
#include "FieldSolver.h"
#include "Grid.h"
#include "Vectors.h"
#include "PmlPsatd.h"

namespace pfc {

    class PSATDTimeStraggered : public SpectralFieldSolver<PSATDGridType>
    {

    public:
        PSATDTimeStraggered(PSATDGrid * grid);

        void updateFields();

        void updateHalfB();
        void updateE();

        void setPML(int sizePMLx, int sizePMLy, int sizePMLz);

        void setTimeStep(FP dt);

        ScalarField<complexFP> tmpJx, tmpJy, tmpJz;

    protected:

        PmlSpectral<GridTypes::PSATDGridType>* getPml() {
            return (PmlSpectral<GridTypes::PSATDGridType>*)pml.get();
        }

        void saveJ();
    };

    inline PSATDTimeStraggered::PSATDTimeStraggered(PSATDGrid* _grid) :
        SpectralFieldSolver<GridTypes::PSATDGridType>(_grid),
        tmpJx(FourierTransform::getSizeOfComplex(_grid->Jx.getSize())),
        tmpJy(FourierTransform::getSizeOfComplex(_grid->Jy.getSize())),
        tmpJz(FourierTransform::getSizeOfComplex(_grid->Jz.getSize()))
    {
        updateDims();
        updateInternalDims();
    }

    inline void PSATDTimeStraggered::setPML(int sizePMLx, int sizePMLy, int sizePMLz)
    {
        pml.reset(new PmlPsatd(this, Int3(sizePMLx, sizePMLy, sizePMLz)));
        updateInternalDims();
    }

    inline void PSATDTimeStraggered::setTimeStep(FP dt)
    {
        if (grid->setTimeStep(dt)) {
            complexGrid->setTimeStep(dt);
            if (pml.get()) pml.reset(new PmlPsatd(this, pml->sizePML));
        }
    }

    inline void PSATDTimeStraggered::saveJ()
    {
        tmpJx.toVector().assign(complexGrid->Jx.toVector().begin(), complexGrid->Jx.toVector().end());
        tmpJy.toVector().assign(complexGrid->Jy.toVector().begin(), complexGrid->Jy.toVector().end());
        tmpJz.toVector().assign(complexGrid->Jz.toVector().begin(), complexGrid->Jz.toVector().end());
    }

    inline void PSATDTimeStraggered::updateFields()
    {
        doFourierTransform(RtoC);

        if (pml.get()) getPml()->updateBSplit();
        updateHalfB();

        if (pml.get()) getPml()->updateESplit();
        updateE();

        if (pml.get()) getPml()->updateBSplit();
        updateHalfB();

        saveJ();
        doFourierTransform(CtoR);

        if (pml.get()) getPml()->doSecondStep();
    }

    inline void PSATDTimeStraggered::updateHalfB()
    {
        const Int3 begin = updateComplexBAreaBegin;
        const Int3 end = updateComplexBAreaEnd;
        double dt = grid->dt*0.5;
#pragma omp parallel for
        for (int i = begin.x; i < end.x; i++)
            for (int j = begin.y; j < end.y; j++)
            {
//#pragma omp simd
                for (int k = begin.z; k < end.z; k++)
                {
                    FP3 K = getWaveVector(Int3(i, j, k));
                    FP normK = K.norm();
                    if (normK == 0) {
                        continue;
                    }
                    K = K / normK;

                    ComplexFP3 E(complexGrid->Ex(i, j, k), complexGrid->Ey(i, j, k), complexGrid->Ez(i, j, k));
                    ComplexFP3 J(complexGrid->Jx(i, j, k), complexGrid->Jy(i, j, k), complexGrid->Jz(i, j, k)),
                        prevJ(tmpJx(i, j, k), tmpJy(i, j, k), tmpJz(i, j, k));
                    ComplexFP3 crossKE = cross((ComplexFP3)K, E);
                    ComplexFP3 crossKJ = cross((ComplexFP3)K, J-prevJ);

                    FP S = sin(normK*constants::c*dt*0.5), C = cos(normK*constants::c*dt*0.5);
                    complexFP coeff1 = 2 * complexFP::i()*S, coeff2 = complexFP::i() * ((1 - C) / (normK*constants::c));

                    complexGrid->Bx(i, j, k) += -coeff1 * crossKE.x + coeff2 * crossKJ.x;
                    complexGrid->By(i, j, k) += -coeff1 * crossKE.y + coeff2 * crossKJ.y;
                    complexGrid->Bz(i, j, k) += -coeff1 * crossKE.z + coeff2 * crossKJ.z;
                }
            }
    }

    inline void PSATDTimeStraggered::updateE()
    {
        const Int3 begin = updateComplexEAreaBegin;
        const Int3 end = updateComplexEAreaEnd;
        double dt = grid->dt;
#pragma omp parallel for
        for (int i = begin.x; i < end.x; i++)
            for (int j = begin.y; j < end.y; j++)
            {
//#pragma omp simd
                for (int k = begin.z; k < end.z; k++)
                {
                    FP3 K = getWaveVector(Int3(i, j, k));
                    FP normK = K.norm();
                    if (normK == 0) {
                        complexGrid->Ex(i, j, k) += dt*complexGrid->Jx(i, j, k);
                        complexGrid->Ey(i, j, k) += dt*complexGrid->Jy(i, j, k);
                        complexGrid->Ez(i, j, k) += dt*complexGrid->Jz(i, j, k);
                        continue;
                    }
                    K = K / normK;

                    ComplexFP3 B(complexGrid->Bx(i, j, k), complexGrid->By(i, j, k), complexGrid->Bz(i, j, k));
                    ComplexFP3 J(complexGrid->Jx(i, j, k), complexGrid->Jy(i, j, k), complexGrid->Jz(i, j, k));
                    ComplexFP3 crossKB = cross((ComplexFP3)K, B);
                    ComplexFP3 Jl = (ComplexFP3)K * dot((ComplexFP3)K, J);

                    FP S = sin(normK*constants::c*dt*0.5);
                    complexFP coeff1 = 2 * complexFP::i()*S, coeff2 = 2 * S / (normK*constants::c),
                        coeff3 = coeff2 - dt;

                    complexGrid->Ex(i, j, k) += coeff1 * crossKB.x - coeff2 * J.x + coeff3 * Jl.x;
                    complexGrid->Ey(i, j, k) += coeff1 * crossKB.y - coeff2 * J.y + coeff3 * Jl.y;
                    complexGrid->Ez(i, j, k) += coeff1 * crossKB.z - coeff2 * J.z + coeff3 * Jl.z;
                }
            }
    }


    class PSATD : SpectralFieldSolver<PSATDGridType>
    {
    public:
        PSATD(PSATDGrid* grid);

        void updateFields();

        void updateEB();

        void setPML(int sizePMLx, int sizePMLy, int sizePMLz);

        void setTimeStep(FP dt);

    private:

        PmlSpectralTimeStraggered<GridTypes::PSATDGridType>* getPml() {
            return (PmlSpectralTimeStraggered<GridTypes::PSATDGridType>*)pml.get();
        }

    };

    inline PSATD::PSATD(PSATDGrid* _grid) :
        SpectralFieldSolver<GridTypes::PSATDGridType>(_grid)
    {
        updateDims();
        updateInternalDims();
    }

    inline void PSATD::setPML(int sizePMLx, int sizePMLy, int sizePMLz)
    {
        pml.reset(new PmlPsatd(this, Int3(sizePMLx, sizePMLy, sizePMLz)));
        updateInternalDims();
    }

    inline void PSATD::setTimeStep(FP dt)
    {
        if (grid->setTimeStep(dt)) {
            complexGrid->setTimeStep(dt);
            if (pml.get()) pml.reset(new PmlPsatd(this, pml->sizePML));
        }
    }

    inline void PSATD::updateFields() {
        doFourierTransform(RtoC);

        if (pml.get()) getPml()->updateBSplit();
        updateEB();
        if (pml.get()) getPml()->updateESplit();
        updateEB();
        if (pml.get()) getPml()->updateBSplit();

        doFourierTransform(CtoR);

        if (pml.get()) getPml()->doSecondStep();          
    }

    inline void PSATD::updateEB()
    {
        const Int3 begin = updateComplexBAreaBegin;
        const Int3 end = updateComplexBAreaEnd;
        double dt = grid->dt/2;
#pragma omp parallel for
        for (int i = begin.x; i < end.x; i++)
            for (int j = begin.y; j < end.y; j++)
            {
//#pragma omp simd
                for (int k = begin.z; k < end.z; k++)
                {
                    FP3 K = getWaveVector(Int3(i, j, k));
                    FP normK = K.norm();

                    ComplexFP3 E(complexGrid->Ex(i, j, k), complexGrid->Ey(i, j, k), complexGrid->Ez(i, j, k));
                    ComplexFP3 B(complexGrid->Bx(i, j, k), complexGrid->By(i, j, k), complexGrid->Bz(i, j, k));
                    ComplexFP3 J(complexGrid->Jx(i, j, k), complexGrid->Jy(i, j, k), complexGrid->Jz(i, j, k));
                    J = complexFP(4 * constants::pi) * J;

                    if (normK == 0) {
                        complexGrid->Ex(i, j, k) += -J.x;
                        complexGrid->Ey(i, j, k) += -J.y;
                        complexGrid->Ez(i, j, k) += -J.z;
                        continue;
                    }

                    K = K / normK;

                    ComplexFP3 kEcross = cross((ComplexFP3)K, E), kBcross = cross((ComplexFP3)K, B),
                        kJcross = cross((ComplexFP3)K, J);
                    ComplexFP3 Jl = (ComplexFP3)K * dot((ComplexFP3)K, J), El = (ComplexFP3)K * dot((ComplexFP3)K, E);

                    FP S = sin(normK*constants::c*dt), C = cos(normK*constants::c*dt);

                    complexFP coef1E = S * complexFP::i(), coef2E = -S / (normK*constants::c),
                        coef3E = S / (normK*constants::c) - dt;

                    complexGrid->Ex(i, j, k) = C * E.x + coef1E * kBcross.x + (1 - C) * El.x + coef2E * J.x + coef3E * Jl.x;
                    complexGrid->Ey(i, j, k) = C * E.y + coef1E * kBcross.y + (1 - C) * El.y + coef2E * J.y + coef3E * Jl.y;
                    complexGrid->Ez(i, j, k) = C * E.z + coef1E * kBcross.z + (1 - C) * El.z + coef2E * J.z + coef3E * Jl.z;
                
                    complexFP coef1B = -S * complexFP::i(), coef2B = ((1 - C) / (normK*constants::c))*complexFP::i();

                    complexGrid->Bx(i, j, k) = C * B.x + coef1B * kEcross.x + coef2B * kJcross.x;
                    complexGrid->By(i, j, k) = C * B.y + coef1B * kEcross.y + coef2B * kJcross.y;
                    complexGrid->Bz(i, j, k) = C * B.z + coef1B * kEcross.z + coef2B * kJcross.z;
                }
            }
    }
}
