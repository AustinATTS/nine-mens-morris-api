#include "muehle/mini_max/mini_max.h"

#include <numeric>

#include "muehle/mini_max/retro_analysis/solver.h"

namespace muehle {

/* MiniMax class constructor */
mini_max::MiniMax::MiniMax(GameInterface* game,
                           unsigned int max_alpha_beta_search_depth)
    : log{Logger::LogLevel::info, Logger::LogType::both,
          /* filename: */ L"mini_max.log"},
      game{game},
      db{*game, log},
      checker{log, thread_manager, db, *game},
      monitor{this, log},
      ab_solver{log, thread_manager, db, *game},
      rt_solver{log, thread_manager, db, *game} {
  /* Init default values */
  cur_calculated_layer = 0;
  ab_solver.SetSearchDepth(max_alpha_beta_search_depth);

  InitializeCriticalSection(&cs_os_print);
  srand((unsigned int)time(/* timer: */ NULL));
}

/* MiniMax class destructor */
mini_max::MiniMax::~MiniMax() {
  CloseDatabase();
  DeleteCriticalSection(&cs_os_print);
}

unsigned int mini_max::MiniMax::GetNumThreads() {
  return thread_manager.GetNumThreads();
}

/* Called by MAIN thread in p_mini_max->cs_os_print critical section */
bool mini_max::MiniMax::AnyFeshlyCalculatedLayer() {
  return (last_calculated_layer.size() > 0);
}

/* Returns the best choice if the database has been opened and calculates the
 * best choice for that if the database is not open */
bool mini_max::MiniMax::GetBestChoice(unsigned int& choice,
                                      StateInfo& info_about_choices) {
  /* Set global vars */
  return ab_solver.GetBestChoice(choice, info_about_choices);
}

void mini_max::MiniMax::SetSearchDepth(
    unsigned int max_alpha_beta_search_depth) {
  ab_solver.SetSearchDepth(max_alpha_beta_search_depth);
}

bool mini_max::MiniMax::OpenDatabase(std::wstring const& directory,
                                     bool use_comp_file_if_both_exist) {
  file_directory.assign(directory);
  if (file_directory.empty()) {
    file_directory = L"./database/";
  }

  if (db.IsOpen()) {
    return log.Log(Logger::LogLevel::trace,
                   /* message: */ L"Database already open!");
  }
  return db.OpenDatabase(file_directory, use_comp_file_if_both_exist);
}

/* Calculates the database which must be already open */
bool mini_max::MiniMax::CalculateDatabase() {
  /* Locals */
  bool abort_calculation = false;
  last_calculated_layer.clear();

  log.Log(Logger::LogLevel::info, /* message: */ L"*************************");
  log.Log(Logger::LogLevel::info, /* message: */ L"*  Calculate  Database  *");
  log.Log(Logger::LogLevel::info, /* message: */ L"*************************");

  /* Call preparation function of parent class */
  game->PrepareCalculation();

  /* When database not completed, then do it */
  if (db.IsOpen() && !db.IsComplete()) {
    /* Reserve memory */
    last_calculated_layer.clear();
    thread_manager.Reset();

    /* Calc layer after layer, beginning with the last one */
    for (cur_calculated_layer = 0; cur_calculated_layer < db.GetNumLayers();
         cur_calculated_layer++) {
      /* Layer already calculated */
      if (db.IsLayerCompleteAndInFile(cur_calculated_layer)) {
        continue;
      }

      /* Get layers to calculate */
      layers_to_calculate = db.GetPartnerLayers(cur_calculated_layer);
      layers_to_calculate.push_back(cur_calculated_layer);

      /* Remove duplicates */
      sort(/* first: */ layers_to_calculate.begin(),
           /* last: */ layers_to_calculate.end());
      layers_to_calculate.erase(
          /* last: */ unique(/* first: */ layers_to_calculate.begin(),
                             /* last: */ layers_to_calculate.end()),
          /* last: */ layers_to_calculate.end());

      /* Dont calc if neither the layer nor the partner layer has any knots */
      unsigned int total_number_of_knots = std::accumulate(
          /* first: */ layers_to_calculate.begin(),
          /* last: */ layers_to_calculate.end(), /* init: */ 0,
          /* binary_op: */ [&](unsigned int sum, unsigned int cur_layer) {
            return sum + db.GetNumberOfKnots(cur_layer);
          });
      if (total_number_of_knots == 0) {
        continue;
      }

      /* Calc */
      abort_calculation = (!CalcLayer(cur_calculated_layer));

      /* Release memory */
      UnloadDatabase();

      /* Dont save layer when aborted */
      if (abort_calculation) {
        break;
      }

      /* Save header */
      db.SaveHeader();
    }

    /* Dont save layer and header when only preparing layers or when aborting */
    if (!abort_calculation) {
      /* Calc layer statistic */
      monitor.CalcLayerStatistics("satistics.csv");

      /* Save header */
      db.SetAsComplete();
      db.SaveHeader();
    }

    /* Free mem */
    cur_action = Activity::none;
  } else {
    log.Log(Logger::LogLevel::info,
            /* message: */ L"The database is already fully calculated.");
  }

  if (abort_calculation) {
    log.Log(Logger::LogLevel::error,
            /* message: */ L"Layer calculation cancelled or failed!");
    return false;
  }

  /* Output */
  log.Log(Logger::LogLevel::info, /* message: */ L"*************************");
  log.Log(Logger::LogLevel::info, /* message: */ L"* Calculation  Finished *");
  log.Log(Logger::LogLevel::info, /* message: */ L"*************************");
  return true;
}

/* Calculates statistics for a completed database. */
bool mini_max::MiniMax::CalculateStatistics() {
  log.Log(Logger::LogLevel::info, /* message: */ L"*************************");
  log.Log(Logger::LogLevel::info, /* message: */ L"* Calculate  Statistics *");
  log.Log(Logger::LogLevel::info, /* message: */ L"*************************");

  /* Check if database is open and complete */
  if (db.IsOpen() && db.IsComplete()) {
    /* Calculate layer statistics */
    monitor.CalcLayerStatistics("statistics.csv");

    log.Log(Logger::LogLevel::info,
            /* message: */ L"*************************");
    log.Log(Logger::LogLevel::info,
            /* message: */ L"*  Statistics Finished  *");
    log.Log(Logger::LogLevel::info,
            /* message: */ L"*************************");
    return true;
  } else {
    log.Log(Logger::LogLevel::error,
            /* message: */
            L"Database must be open and complete to calculate statistics.");
    return false;
  }
}

bool mini_max::MiniMax::IsCurrentStateInDatabase(unsigned int thread_no) {
  unsigned int layer_num, state_number, sym_op;

  if (!db.IsOpen()) {
    return false;
  } else {
    game->GetLayerAndStateNumber(thread_no, layer_num, state_number, sym_op);
    return db.IsLayerCompleteAndInFile(layer_num);
  }
}

void mini_max::MiniMax::UnloadDatabase() {
  db.Unload();
}

void mini_max::MiniMax::CloseDatabase() {
  db.CloseDatabase();
}

void mini_max::MiniMax::PauseDatabaseCalculation() {
  thread_manager.PauseExecution();
}

void mini_max::MiniMax::CancelDatabaseCalculation() {
  /* When returning from ExecuteParallelLoop() all functions shall quit
   * immediatelly up to CalculateDatabase() */
  thread_manager.CancelExecution();
}

bool mini_max::MiniMax::WasDatabaseCalculationCancelled() {
  return thread_manager.WasExecutionCancelled();
}

/* Called by MAIN thread in p_mini_max->cs_os_print in critical section */
unsigned int mini_max::MiniMax::GetLastCalculatedLayer() {
  if (last_calculated_layer.size() == 0) {
    return game->GetNumberOfLayers();
  }
  unsigned int tmp = last_calculated_layer.front();
  last_calculated_layer.pop_front();
  return tmp;
}

/* Sets the output stream for the logger console output */
bool mini_max::MiniMax::SetOutputStream(std::wostream& the_stream) {
  return log.SetOutputSteam(the_stream);
}

bool mini_max::MiniMax::SetNumThreads(unsigned int num_threads) {
  return thread_manager.SetNumThreads(num_threads);
}

bool mini_max::MiniMax::CalcLayer(unsigned int layer_number) {
  /* Moves can be done reverse, leading to two depth searching trees */
  if (game->ShallRetroAnalysisBeUsed(layer_number)) {
    if (!rt_solver.CalcKnotValuesByRetroAnalysis(layers_to_calculate)) {
      return false;
    }
    /* Use MiniMax algorithm */
  } else {
    if (!ab_solver.CalcKnotValueByAlphaBeta(layers_to_calculate)) {
      return false;
    }
  }

  /* Save layers */
  for (auto layer : layers_to_calculate) {
    db.SaveLayerToFile(layer);
  }

  /* Test layers */
  for (auto layer : layers_to_calculate) {
    if (!checker.TestLayer(layer)) {
      return log.Log(
          Logger::LogLevel::error,
          /* message: */ L"ERROR: Layer calculation cancelled or failed!");
    }
  }

  /* Update output information */
  EnterCriticalSection(&cs_os_print);
  last_calculated_layer.assign(/* first: */ layers_to_calculate.begin(),
                               /* last: */ layers_to_calculate.end());
  LeaveCriticalSection(&cs_os_print);

  /* Everything was ok */
  return true;
}

void mini_max::MiniMax::SetCurrentActivity(Activity new_action) {
  cur_action = new_action;
}

} /* namespace muehle */
