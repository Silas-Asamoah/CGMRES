#include "matrixfree_gmres.hpp"


MatrixFreeGMRES::MatrixFreeGMRES(const int dim_equation, const int k)
{
    dim_equation_ = dim_equation;
    max_dim_krylov_ = k;

    // allocate matrices and vectors
    hessenberg_mat_.resize(max_dim_krylov_+1, max_dim_krylov_+1);
    basis_mat_.resize(dim_equation_, max_dim_krylov_+1);
    current_error_vec_.resize(dim_equation_);
    givens_c_vec_.resize(max_dim_krylov_+1);
    givens_s_vec_.resize(max_dim_krylov_+1);
    g_vec_.resize(max_dim_krylov_+1);
}


void MatrixFreeGMRES::resetParameters(const int dim_equation, const int k)
{
    dim_equation_ = dim_equation;
    max_dim_krylov_ = k;

    // allocate matrices and vectors
    hessenberg_mat_.resize(max_dim_krylov_+1, max_dim_krylov_+1);
    basis_mat_.resize(dim_equation_, max_dim_krylov_+1);
    current_error_vec_.resize(dim_equation_);
    givens_c_vec_.resize(max_dim_krylov_+1);
    givens_s_vec_.resize(max_dim_krylov_+1);
    g_vec_.resize(max_dim_krylov_+1);
}



void MatrixFreeGMRES::forwardDifferenceGMRES(const double time_param, const Eigen::VectorXd& state_vec, const Eigen::VectorXd& current_solution_vec, Eigen::Ref<Eigen::VectorXd> solution_update_vec)
{
    for(int i=0; i<=max_dim_krylov_; i++){
        givens_c_vec_(i) = 0.0;
        givens_s_vec_(i) = 0.0;
        g_vec_(i) = 0.0;
    }

    nonlinearEquation(time_param, state_vec, current_solution_vec, current_error_vec_);
    g_vec_(0) = current_error_vec_.norm();
    basis_mat_.col(0) = current_error_vec_ / g_vec_(0);

    // k : the dimension of the current Krylov subspace
    int k;
    for(k=0; k<max_dim_krylov_; k++){
        forwardDifferenceEquation(time_param, state_vec, current_solution_vec, basis_mat_.col(k), basis_mat_.col(k+1));
        for(int j=0; j<=k; j++){
            hessenberg_mat_(j,k) = basis_mat_.col(k+1).dot(basis_mat_.col(j));
            basis_mat_.col(k+1) -= hessenberg_mat_(j,k) * basis_mat_.col(j);
        }
        hessenberg_mat_(k+1,k) = basis_mat_.col(k+1).norm();

        if(hessenberg_mat_(k+1,k) != 0){
            basis_mat_.col(k+1) = basis_mat_.col(k+1) / hessenberg_mat_(k+1,k);
        }
        else {
            std::cout << "The modified Gram-Schmidt breakdown at k=" << k << std::endl;
            break;
        }

        // Givens Rotation for the Lieast Squares Problem
        for(int j=0; j<k; j++){
            givensRotation(hessenberg_mat_.col(k), j);
        }
        double nu = std::sqrt(hessenberg_mat_(k,k)*hessenberg_mat_(k,k) + hessenberg_mat_(k+1,k)*hessenberg_mat_(k+1,k));
        if(nu != 0) {
            givens_c_vec_(k) = hessenberg_mat_(k,k) / nu;
            givens_s_vec_(k) = - hessenberg_mat_(k+1,k) / nu;
            hessenberg_mat_(k,k) = givens_c_vec_(k) * hessenberg_mat_(k,k) - givens_s_vec_(k) * hessenberg_mat_(k+1,k);
            hessenberg_mat_(k+1,k) = 0;
            givensRotation(g_vec_,k);
        }
        else{
            std::cout << "Lose orthogonality of the basis of the Krylov subspace!!" << std::endl;
        }
    }

    // solve H * y = g : H is an upper hessenberg matrix 
    for(int i=k-1; i>=0; i--){
        double tmp=g_vec_(i);
        for(int j=i+1; j<k; j++){
            tmp -= hessenberg_mat_(i,j) * givens_c_vec_(j);
        }
        givens_c_vec_(i) = tmp / hessenberg_mat_(i,i);
    }
    for(int i=0; i<dim_equation_; i++){
        double tmp=0;
        for(int j=0; j<k; j++){
            tmp += basis_mat_(i,j) * givens_c_vec_(j);
        }
        solution_update_vec(i) += tmp;
    }
}


void MatrixFreeGMRES::givensRotation(Eigen::Ref<Eigen::VectorXd> column_vec, const int i_column)
{
    double tmp1, tmp2;

    tmp1 = givens_c_vec_(i_column) * column_vec(i_column) - givens_s_vec_(i_column) * column_vec(i_column+1);
    tmp2 = givens_s_vec_(i_column) * column_vec(i_column) + givens_c_vec_(i_column) * column_vec(i_column+1);

    column_vec(i_column) = tmp1;
    column_vec(i_column+1) = tmp2;
}