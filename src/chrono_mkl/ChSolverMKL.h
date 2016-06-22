//
// PROJECT CHRONO - http://projectchrono.org
//
// Copyright (c) 2010 Alessandro Tasora
// All rights reserved.
//
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file at the top level of the distribution
// and at http://projectchrono.org/license-chrono.txt.
//

#ifndef CHSOLVERMKL_H
#define CHSOLVERMKL_H

#include "chrono/core/ChMatrixDynamic.h"
#include "chrono/solver/ChSolver.h"
#include "chrono/solver/ChSystemDescriptor.h"
#include "chrono_mkl/ChMklEngine.h"

namespace chrono {

/// @addtogroup mkl_module
/// @{

/// Class that wraps the Intel MKL Pardiso parallel direct solver.
/// It can solve linear systems, but not VI and complementarity problems.
class ChApiMkl ChSolverMKL : public ChSolver {
    // Chrono RTTI, needed for serialization
    CH_RTTI(ChSolverMKL, ChSolver);

  private:
    size_t solver_call;
    ChCSR3Matrix matCSR3;
    ChMatrixDynamic<double> rhs;
    ChMatrixDynamic<double> sol;
    ChMatrixDynamic<double> res;
    ChMklEngine mkl_engine;
    size_t n;
    size_t nnz;

    bool sparsity_pattern_lock;
    bool use_perm;
    bool use_rhs_sparsity;

  public:
    ChSolverMKL();
    virtual ~ChSolverMKL() {}

    ChMklEngine& GetMklEngine() { return mkl_engine; }
    ChCSR3Matrix& GetMatrix() { return matCSR3; }

    /// Enable/disable locking the sparsity pattern. 
    /// If on_off is set to true, then the sparsity pattern of the problem matrix is assumed
    /// to be unchanged from call to call.
    void SetSparsityPatternLock(bool on_off) { sparsity_pattern_lock = on_off; }

    void UsePermutationVector(bool on_off) { use_perm = on_off; }

    void LeverageRhsSparsity(bool on_off) { use_rhs_sparsity = on_off; }

    void SetPreconditionedCGS(bool on_off, int L) { mkl_engine.SetPreconditionedCGS(on_off, L); }

    void SetMatrixNNZ(size_t nnz_input) { nnz = nnz_input; };

    /// Solve using the MKL Pardiso sparse direct solver.
    /// It uses the matrix factorization obtained at the last call to Setup().
    virtual double Solve(ChSystemDescriptor& sysd) override;

    /// Perform the solver setup operations.
    /// For the MKL solver, this means assembling and factorizing the system matrix.
    /// Returns true if successful and false otherwise.
    virtual bool Setup(ChSystemDescriptor& sysd) override;

    //
    // SERIALIZATION
    //

    virtual void ArchiveOUT(ChArchiveOut& marchive) override {
        // version number
        marchive.VersionWrite(1);
        // serialize parent class
        ChSolver::ArchiveOUT(marchive);
        // serialize all member data:
        marchive << CHNVP(sparsity_pattern_lock);
        marchive << CHNVP(use_perm);
        marchive << CHNVP(use_rhs_sparsity);
    }

    /// Method to allow de serialization of transient data from archives.
    virtual void ArchiveIN(ChArchiveIn& marchive) override {
        // version number
        int version = marchive.VersionRead();
        // deserialize parent class
        ChSolver::ArchiveIN(marchive);
        // stream in all member data:
        marchive >> CHNVP(sparsity_pattern_lock);
        marchive >> CHNVP(use_perm);
        marchive >> CHNVP(use_rhs_sparsity);
    }
};

/// @} mkl_module

}  // end namespace chrono

#endif
