#ifndef NETKET_JSON_OUTPUT_WRITER_HPP
#define NETKET_JSON_OUTPUT_WRITER_HPP

#include <cassert>
#include <fstream>

//#include <nonstd/optional.hpp>

#include "Machine/abstract_machine.hpp"
#include "Stats/obs_manager.hpp"
#include "Utils/json_dumps.hpp"
#include "Utils/json_helper.hpp"

#ifndef NDEBUG
#include <mpi.h>
#endif

namespace netket {

/**
 * The JsonOutputWriter class is responsible for writing log files containing
 * observable and wave function data.
 */
class JsonOutputWriter {
 public:
  static JsonOutputWriter FromJson(const json& pars) {
    const std::string filebase = FieldVal(pars, "OutputFile");
    const int freqbackup = FieldOrDefaultVal(pars, "SaveEvery", 50);

    return JsonOutputWriter(filebase + ".log", filebase + ".wf", freqbackup);
  }

  /**
   * Construct an output writer writing JSON data to two files.
   * @param log The filename for the log data (time, energy, and other
   * expectation values)
   * @param wf The filename for the wavefunction data, which is written every \p
   * freqbackup time steps.
   * @param Frequency for saving the wavefunction. Must be positive or zero (in
   * which case no states are save).
   */
  JsonOutputWriter(const std::string& log, const std::string& wf,
                   int freqbackup)
      : log_stream_(log), wf_stream_name_(wf), freqbackup_(freqbackup) {
#ifndef NDEBUG
    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    DebugMessage() << "JsonOutputWriter constructed at MPI rank " << rank
                   << std::endl;
#endif
    assert(freqbackup >= 0);
    log_stream_ << _s_start;
    log_stream_.flush();
  }

  /**
   * Write data about the current iteration to the log file.
   * @param iteration The index of the current iteration.
   * @param observable_data JSON represention of the observable data.
   * @param time Optionally, the current simulation time which will be included
   * in the log.
   */
  void WriteLog(int iteration, const json& observable_data,
                std::optional<double> time = std::nullopt) {
    json data = observable_data;
    data["Iteration"] = iteration;
    if (time.has_value()) {
      data["Time"] = time.value();
    }

    // Go back to replace the last characters by a comma and a new line.
    // This turns this:
    //     { <previous data...> }
    //     ]}
    // into this:
    //     { <previous data...> },
    //
    // This makes sure that the log file remains valid JSON after every step.
    const long pos = log_stream_.tellp();
    log_stream_.seekp(pos - 4);
    // Only write a comma after the first iteration
    if (pos > static_cast<std::ptrdiff_t>(_s_start.size())) {
      log_stream_ << ",\n";
    } else {
      log_stream_ << "\n";
    }

    log_stream_ << data;
    log_stream_ << " \n]}";
    log_stream_.flush();
  }

  /**
   * Write state data to the logfile, if iteration is divisible by the backup
   * frequency.
   */
  template <class State>
  void WriteState(int iteration, const State& state) {
    if (freqbackup_ == 0) {
      return;
    }
    if (iteration % freqbackup_ == 0) {
      std::ofstream wf_stream{wf_stream_name_};
      SaveState_Impl(wf_stream, state);
    }
  }

 private:
  // Member functions functions for saving the state.
  // The first overload works for classes inheriting from AbstractMachine, the
  // second one for Eigen matrices.
  template <typename T>
  void SaveState_Impl(std::ofstream& stream, const AbstractMachine<T>& state) {
    state.Save(stream);
  }

  template <typename T, int S1, int S2>
  void SaveState_Impl(std::ofstream& stream,
                      const Eigen::Matrix<T, S1, S2>& state) {
    json j;
    j["StateVector"] = state;
    stream << j << std::endl;
  }

  static std::string _s_start;

  std::ofstream log_stream_;
  std::string wf_stream_name_;

  int freqbackup_;
};

std::string JsonOutputWriter::_s_start = "{\"Output\": [  ]}";

}  // namespace netket

#endif  // NETKET_JSON_OUTPUT_WRITER_HPP
