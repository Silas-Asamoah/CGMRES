#include "cgmres_simulator.hpp"


void nmpcsim::simulation(ContinuationGMRES nmpc_solver, const Eigen::VectorXd& initial_state_vec, const double start_time, const double end_time, const double sampling_period, const std::string savefile_name)
{
    NMPCModel model;
    NumericalIntegrator integrator;
    Eigen::VectorXd current_state_vec(model.dimState()), next_state_vec(model.dimState()), control_input_vec(model.dimControlInput());
    std::chrono::system_clock::time_point start_clock, end_clock;

    std::string save_dir_name("simulation_result");
    makeSaveDir(save_dir_name);
    std::ofstream state_data(save_dir_name + "/" + savefile_name + "_state.dat");
    std::ofstream control_input_data(save_dir_name + "/" + savefile_name + "_control_input.dat");
    std::ofstream error_data(save_dir_name + "/" + savefile_name + "_error.dat");
    std::ofstream conditions_data(save_dir_name + "/" + savefile_name + "_conditions.dat");

    double total_time = 0;
    current_state_vec = initial_state_vec;
    control_input_vec = nmpc_solver.getControlInput();
    std::cout << "Start simulation" << std::endl;
    for(double current_time=start_time; current_time<end_time; current_time+= sampling_period){
        saveData(state_data, control_input_data, error_data, current_time, current_state_vec, control_input_vec, nmpc_solver.getError(current_time, current_state_vec));

        // Compute the next state vector using the 4th Runge-Kutta-Gill method.
        next_state_vec = integrator.rungeKuttaGill(current_time, current_state_vec, control_input_vec, sampling_period);

        // Update the solution and measure computational time.
        start_clock = std::chrono::system_clock::now();
        nmpc_solver.controlUpdate(current_time, sampling_period, current_state_vec, control_input_vec);
        end_clock = std::chrono::system_clock::now();

        // Convert computational time to seconds.
        double step_time = std::chrono::duration_cast<std::chrono::microseconds>(end_clock-start_clock).count();
        step_time *= 1e-06;
        total_time += step_time;

        current_state_vec = next_state_vec;
    }
    std::cout << "End simulation" << std::endl;
    std::cout << "Total CPU time for control update: " << total_time << " [sec]" << std::endl;
    
    // Save simulation conditions.
    conditions_data << "simulation name: " << savefile_name << "\n";
    conditions_data << "simulation time: " << end_time-start_time << " [sec]\n";
    conditions_data << "CPU time for control update (total): " << total_time << " [sec]\n";
    conditions_data << "sampling time: " << sampling_period << " [sec]\n";
    conditions_data << "CPU time for control update (1step): " << total_time/((int)( (end_time-start_time)/(sampling_period))) << " [sec]\n";

    state_data.close();
    control_input_data.close();
    error_data.close();
    conditions_data.close();
}