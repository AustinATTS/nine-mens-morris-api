#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>
#include <limits>
#include <regex>
#include <sstream>
#include <stdexcept>

#include "muehle/api/muehle_bridge.h"

namespace muehle {

std::string Trim(const std::string& text) {
  const auto first = text.find_first_not_of(/* s: */ " \t\r\n");
  if (first == std::string::npos) {
    return {};
  }
  const auto last = text.find_last_not_of(/* s: */ " \t\r\n");
  return text.substr(/* pos: */ first, /* n: */ last - first + 1);
}

std::string UnescapeJsonString(std::string value) {
  std::string result;
  result.reserve(/* res: */ value.size());
  for (std::size_t index = 0; index < value.size(); ++index) {
    if (value[index] == '\\' && index + 1 < value.size()) {
      const char next = value[++index];
      switch (next) {
        case '\\':
          result.push_back(/* c: */ '\\');
          break;
        case '"':
          result.push_back(/* c: */ '"');
          break;
        case '/':
          result.push_back(/* c: */ '/');
          break;
        case 'b':
          result.push_back(/* c: */ '\b');
          break;
        case 'f':
          result.push_back(/* c: */ '\f');
          break;
        case 'n':
          result.push_back(/* c: */ '\n');
          break;
        case 'r':
          result.push_back(/* c: */ '\r');
          break;
        case 't':
          result.push_back(/* c: */ '\t');
          break;
        default:
          result.push_back(next);
          break;
      }
    } else {
      result.push_back(value[index]);
    }
  }
  return result;
}

bool ExtractUnsigned(const std::string& input, const std::string& key,
                     unsigned int& value) {
  const std::regex pattern(/* s: */ "\"" + key + "\"\\s*:\\s*(\\d+)");
  std::smatch match;
  if (!std::regex_search(input, match, pattern)) {
    return false;
  }
  try {
    const auto parsed = std::stoull(match[1].str());
    if (parsed > std::numeric_limits<unsigned int>::max()) {
      return false;
    }
    value = static_cast<unsigned int>(parsed);
  } catch (const std::exception&) {
    return false;
  }
  return true;
}

bool ExtractBool(const std::string& input, const std::string& key,
                 bool& value) {
  const std::regex pattern(/* s: */ "\"" + key + "\"\\s*:\\s*(true|false)");
  std::smatch match;
  if (!std::regex_search(input, match, pattern)) {
    return false;
  }
  value = (match[1].str() == "true");
  return true;
}

bool ExtractString(const std::string& input, const std::string& key,
                   std::string& value) {
  const std::regex pattern(/* s: */ "\"" + key +
                           "\"\\s*:\\s*\"((?:\\\\.|[^\"])*)\"");
  std::smatch match;
  if (!std::regex_search(input, match, pattern)) {
    return false;
  }
  value = UnescapeJsonString(/* value: */ match[1].str());
  return true;
}

bool ExtractBoard(const std::string& input,
                  std::array<PlayerId, FieldStruct::size>& board,
                  std::string& error) {
  const std::regex pattern(/* p: */ "\"(board|field)\"\\s*:\\s*\\[(.*?)\\]");
  std::smatch match;
  if (!std::regex_search(input, match, pattern)) {
    error = "Missing board array. Expected 24 integers in 'board' or 'field'.";
    return false;
  }

  std::stringstream stream(match[2].str());
  std::string token;
  std::size_t index = 0;
  while (std::getline(stream, token, /* delim: */ ',')) {
    token = Trim(token);
    if (token.empty()) {
      continue;
    }
    if (index >= FieldStruct::size) {
      error = "Board array must contain exactly 24 entries.";
      return false;
    }
    unsigned int stone_value = 0;
    try {
      std::size_t parsed_characters = 0;
      const auto parsed = std::stoul(token, &parsed_characters);
      if (parsed_characters != token.size()) {
        throw std::invalid_argument("trailing characters");
      }
      stone_value = static_cast<unsigned int>(parsed);
    } catch (const std::exception&) {
      error = "Board array values must be integers: 0, 1, or 2.";
      return false;
    }
    switch (stone_value) {
      case 0:
        board[index] = PlayerId::square_is_free;
        break;
      case 1:
        board[index] = PlayerId::player_one;
        break;
      case 2:
        board[index] = PlayerId::player_two;
        break;
      default:
        error = "Board array values must be 0, 1, or 2.";
        return false;
    }
    ++index;
  }

  if (index != FieldStruct::size) {
    error = "Board array must contain exactly 24 entries.";
    return false;
  }

  return true;
}

bool ParseCurrentPlayer(const std::string& input, PlayerId& player) {
  unsigned int player_value = 0;
  if (ExtractUnsigned(input, /* key: */ "current_player", player_value) ||
      ExtractUnsigned(input, /* key: */ "player", player_value)) {
    player = (player_value == 2) ? PlayerId::player_two : PlayerId::player_one;
    return true;
  }

  std::string player_text;
  if (ExtractString(input, /* key: */ "current_player",
                    /* value: */ player_text) ||
      ExtractString(input, /* key: */ "player", /* value: */ player_text)) {
    for (auto& character : player_text) {
      character = static_cast<char>(
          std::tolower(static_cast<unsigned char>(character)));
    }
    if (player_text == "2" || player_text == "player_two" ||
        player_text == "white" || player_text == "o") {
      player = PlayerId::player_two;
    } else {
      player = PlayerId::player_one;
    }
    return true;
  }

  player = PlayerId::player_one;
  return true;
}

std::string PlayerToString(PlayerId player) {
  switch (player) {
    case PlayerId::player_one:
      return "player_one";
    case PlayerId::player_two:
      return "player_two";
    case PlayerId::square_is_free:
      return "square_is_free";
    case PlayerId::player_one_warning:
      return "player_one_warning";
    case PlayerId::player_two_warning:
      return "player_two_warning";
    case PlayerId::player_both_warning:
      return "player_both_warning";
    default:
      return "invalid";
  }
}

std::string ShortValueToString(unsigned int short_value) {
  switch (short_value) {
    case mini_max::SKV_VALUE_GAME_WON:
      return "game_won";
    case mini_max::SKV_VALUE_GAME_DRAWN:
      return "game_drawn";
    case mini_max::SKV_VALUE_GAME_LOST:
      return "game_lost";
    default:
      return "invalid";
  }
}

std::string EscapeJson(const std::string& value) {
  std::string result;
  result.reserve(/* res: */ value.size() + 8);
  for (char character : value) {
    switch (character) {
      case '\\':
        result += "\\\\";
        break;
      case '"':
        result += "\\\"";
        break;
      case '\b':
        result += "\\b";
        break;
      case '\f':
        result += "\\f";
        break;
      case '\n':
        result += "\\n";
        break;
      case '\r':
        result += "\\r";
        break;
      case '\t':
        result += "\\t";
        break;
      default:
        result.push_back(character);
        break;
    }
  }
  return result;
}

unsigned int PlayerToIndex(PlayerId player) {
  switch (player) {
    case PlayerId::player_one:
      return 1;
    case PlayerId::player_two:
      return 2;
    default:
      return 0;
  }
}

std::string SerializeResponse(const MuehleBridge::Response& response) {
  std::ostringstream stream;
  stream << "{";
  stream << "\"success\":" << (response.success ? "true" : "false") << ",";
  stream << "\"engine\":\"" << EscapeJson(response.engine) << "\",";
  stream << "\"search_depth\":" << response.search_depth << ",";
  stream << "\"current_player\":\"" << PlayerToString(response.current_player)
         << "\",";
  stream << "\"current_player_id\":" << PlayerToIndex(response.current_player)
         << ",";
  stream << "\"setting_phase\":" << (response.setting_phase ? "true" : "false")
         << ",";
  stream << "\"total_num_stones_missing\":" << response.total_num_stones_missing
         << ",";
  stream << "\"game_has_finished\":"
         << (response.game_has_finished ? "true" : "false") << ",";
  stream << "\"winner\":\"" << PlayerToString(response.winner) << "\",";
  stream << "\"winner_id\":" << PlayerToIndex(response.winner) << ",";
  stream << "\"best_move\":{"
         << "\"from\":" << response.best_move.from << ","
         << "\"to\":" << response.best_move.to << ","
         << "\"remove_stone\":" << response.best_move.remove_stone << ","
         << "\"id\":" << response.best_move.GetId() << "},";
  stream << "\"best_choice\":{"
         << "\"short_value\":"
         << static_cast<unsigned int>(response.choice_info.short_value) << ","
         << "\"short_value_label\":\""
         << ShortValueToString(response.choice_info.short_value) << "\",";
  stream << "\"ply_info\":" << response.choice_info.ply_info << ",";
  stream << "\"best_amount_of_plies\":"
         << response.choice_info.best_amount_of_plies << "},";
  stream << "\"choices\":[";
  for (std::size_t choice_index = 0; choice_index < response.choices.size();
       ++choice_index) {
    const auto& choice = response.choices[choice_index];
    if (choice_index > 0) {
      stream << ",";
    }
    stream << "{";
    stream << "\"possibility_id\":" << choice.possibility_id << ",";
    stream << "\"move\":{"
           << "\"from\":" << choice.move.from << ","
           << "\"to\":" << choice.move.to << ","
           << "\"remove_stone\":" << choice.move.remove_stone << ","
           << "\"id\":" << choice.move.GetId() << "},";
    stream << "\"short_value\":" << choice.short_value << ",";
    stream << "\"short_value_label\":\""
           << ShortValueToString(choice.short_value) << "\",";
    stream << "\"ply_info\":" << choice.ply_info << ",";
    stream << "\"freq_values_sub_moves\":[";
    for (std::size_t value_index = 0;
         value_index < choice.freq_values_sub_moves.size(); ++value_index) {
      if (value_index > 0) {
        stream << ",";
      }
      stream << choice.freq_values_sub_moves[value_index];
    }
    stream << "]}";
  }
  stream << "]";
  if (!response.error.empty()) {
    stream << ",\"error\":\"" << EscapeJson(response.error) << "\"";
  } else {
    stream << ",\"error\":null";
  }
  stream << "}";
  return stream.str();
}

MuehleBridge::Request ParseRequest(const std::string& input,
                                   std::string& error) {
  MuehleBridge::Request request;
  if (!ExtractBoard(input, request.board, error)) {
    return request;
  }
  ExtractBool(input, /* key: */ "setting_phase", request.setting_phase);
  ExtractUnsigned(input, /* key: */ "total_num_stones_missing",
                  request.total_num_stones_missing);
  ExtractUnsigned(input, /* key: */ "search_depth", request.search_depth);
  ParseCurrentPlayer(input, request.current_player);
  return request;
}

} /* namespace muehle */

int main(int argc, char** argv) {
  const std::string input = [&]() {
    if (argc > 1) {
      const std::string candidate = argv[1];
      std::ifstream file(candidate);
      if (file.good()) {
        std::ostringstream stream;
        stream << file.rdbuf();
        return stream.str();
      }
      return candidate;
    }
    std::ostringstream stream;
    stream << std::cin.rdbuf();
    return stream.str();
  }();

  std::string parse_error;
  const muehle::MuehleBridge::Request request =
      muehle::ParseRequest(input, parse_error);

  if (!parse_error.empty()) {
    muehle::MuehleBridge::Response error_response;
    error_response.error = parse_error;
    std::cout << SerializeResponse(error_response) << std::endl;
    return 1;
  }

  std::ostringstream coutSink;
  std::wostringstream wcoutSink;
  auto* const old_cout_buffer = std::cout.rdbuf(/* sb: */ coutSink.rdbuf());
  auto* const old_wcout_buffer = std::wcout.rdbuf(/* sb: */ wcoutSink.rdbuf());

  muehle::MuehleBridge::Response response;
  {
    muehle::MuehleBridge bridge;
    bridge.SetSearchDepth(request.search_depth);
    response = bridge.Evaluate(request);
  }

  std::cout.rdbuf(old_cout_buffer);
  std::wcout.rdbuf(old_wcout_buffer);
  std::cout << SerializeResponse(response) << std::endl;
  return response.success ? 0 : 1;
}