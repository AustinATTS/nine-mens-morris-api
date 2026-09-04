#include "muehle/utils/my_string.h"

namespace muehle {

/* Empty constructor */
MyString::MyString() {}

/* Constructor for ASCII strings */
MyString::MyString(const char* c_str) {
  Assign(c_str);
}

/* Constructor for wide char strings */
MyString::MyString(const WCHAR* c_str) {
  Assign(c_str);
}

/* Destructor */
MyString::~MyString() {
  if (str_a != nullptr) {
    delete[] str_a;
    str_a = nullptr;
  }
  if (str_w != nullptr) {
    delete[] str_w;
    str_w = nullptr;
  }
  str_w = nullptr;
  str_a = nullptr;
  length = 0;
  reserved = 0;
}

/* Returns the ascii string */
const char* MyString::CStrA() {
  return str_a;
}

/* Returns the wide char string */
const WCHAR* MyString::CStrW() {
  return str_w;
}

/* Set the string to a new ascii string */
MyString& MyString::Assign(const char* c_str) {
  /* Locals */
  size_t new_length = strlen(c_str);
  size_t new_reserved = new_length * 2;

  if (reserved < new_reserved) {
    this->~MyString();
  }
  if (str_a == nullptr) {
    str_a = new char[new_reserved];
  }
  if (str_w == nullptr) {
    str_w = new WCHAR[new_reserved];
  }

  reserved = new_reserved;
  length = new_length;

  strcpy(str_a, c_str);
  mbstowcs(str_w, c_str, /* n: */ new_length + 1);

  return *this;
}

/* Set the string to a new wide char string */
MyString& MyString::Assign(const WCHAR* c_str) {
  /* Locals */
  size_t new_length = wcslen(c_str);
  size_t new_reserved = new_length * 2;

  if (reserved < new_reserved) {
    this->~MyString();
  }
  if (str_a == nullptr) {
    str_a = new char[new_reserved];
  }
  if (str_w == nullptr) {
    str_w = new WCHAR[new_reserved];
  }

  reserved = new_reserved;
  length = new_length;

  wcscpy(str_w, c_str);
  wcstombs(str_a, c_str, /* n: */ new_length + 1);

  return *this;
}

/* This function reads in a table of floating point values faster that "cin" */
bool ReadAsciiData(HANDLE h_file, double* p_data, unsigned int num_values,
                   unsigned char decimal_separator,
                   unsigned char column_separator) {
  /* Check input */
  if (p_data == nullptr) {
    return false;
  }
  if (num_values == 0) {
    return false;
  }
  if (h_file == nullptr) {
    return false;
  }

  /* Constants */
  const unsigned int max_value_length_in_bytes = 32;
  const unsigned int buffer_size = 1000;

  /* Locals */
  DWORD dw_bytes_read;
  unsigned char buffer[buffer_size];
  unsigned char* cur_byte = &buffer[0];
  unsigned int cur_read_value = 0;
  unsigned int actual_buffer_size = 0;
  unsigned int cur_buffer_pos = 0;
  unsigned int decimal_pos = 0;
  int integral_value = 0;   /* Only allows 8 digits before decimal place */
  int fractional_value = 0; /* Only 8 decimal places allowed */
  int exponential_value = 1;
  bool val_is_negative = false;
  bool exp_is_negative = false;
  bool decimal_place = false;
  bool exponent = false;
  double fractional_factor[] = {0,          0.1,         0.01,        0.001,
                                0.0001,     0.00001,     0.000001,    0.0000001,
                                0.00000001, 0.000000001, 0.0000000001};

  /* Read each value */
  do {
    /* Read from buffer if necessary */
    if (cur_buffer_pos >= buffer_size - max_value_length_in_bytes) {
      memcpy(&buffer[0], &buffer[cur_buffer_pos],
             /* n: */ buffer_size - cur_buffer_pos);
      ReadFile(h_file, /* buf: */ &buffer[buffer_size - cur_buffer_pos],
               cur_buffer_pos, &dw_bytes_read, nullptr);
      actual_buffer_size = buffer_size - cur_buffer_pos + dw_bytes_read;
      cur_buffer_pos = 0;
      cur_byte = &buffer[cur_buffer_pos];
    }

    /* Process current byte */
    switch (*cur_byte) {
      case '-':
        if (exponent) {
          exp_is_negative = true;
        } else {
          val_is_negative = true;
        }
        break;
      case '+': /* ignore */
        break;
      case 'e':
      case 'E':
        exponent = true;
        decimal_place = false;
        break;
      case '0':
        if (decimal_place) {
          fractional_value *= 10;
          fractional_value += 0;
          decimal_pos++;
        } else if (exponent) {
          exponential_value *= 10;
          exponential_value += 0;
        } else {
          integral_value *= 10;
          integral_value += 0;
        }
        break;
      case '1':
        if (decimal_place) {
          fractional_value *= 10;
          fractional_value += 1;
          decimal_pos++;
        } else if (exponent) {
          exponential_value *= 10;
          exponential_value += 1;
        } else {
          integral_value *= 10;
          integral_value += 1;
        }
        break;
      case '2':
        if (decimal_place) {
          fractional_value *= 10;
          fractional_value += 2;
          decimal_pos++;
        } else if (exponent) {
          exponential_value *= 10;
          exponential_value += 2;
        } else {
          integral_value *= 10;
          integral_value += 2;
        }
        break;
      case '3':
        if (decimal_place) {
          fractional_value *= 10;
          fractional_value += 3;
          decimal_pos++;
        } else if (exponent) {
          exponential_value *= 10;
          exponential_value += 3;
        } else {
          integral_value *= 10;
          integral_value += 3;
        }
        break;
      case '4':
        if (decimal_place) {
          fractional_value *= 10;
          fractional_value += 4;
          decimal_pos++;
        } else if (exponent) {
          exponential_value *= 10;
          exponential_value += 4;
        } else {
          integral_value *= 10;
          integral_value += 4;
        }
        break;
      case '5':
        if (decimal_place) {
          fractional_value *= 10;
          fractional_value += 5;
          decimal_pos++;
        } else if (exponent) {
          exponential_value *= 10;
          exponential_value += 5;
        } else {
          integral_value *= 10;
          integral_value += 5;
        }
        break;
      case '6':
        if (decimal_place) {
          fractional_value *= 10;
          fractional_value += 6;
          decimal_pos++;
        } else if (exponent) {
          exponential_value *= 10;
          exponential_value += 6;
        } else {
          integral_value *= 10;
          integral_value += 6;
        }
        break;
      case '7':
        if (decimal_place) {
          fractional_value *= 10;
          fractional_value += 7;
          decimal_pos++;
        } else if (exponent) {
          exponential_value *= 10;
          exponential_value += 7;
        } else {
          integral_value *= 10;
          integral_value += 7;
        }
        break;
      case '8':
        if (decimal_place) {
          fractional_value *= 10;
          fractional_value += 8;
          decimal_pos++;
        } else if (exponent) {
          exponential_value *= 10;
          exponential_value += 8;
        } else {
          integral_value *= 10;
          integral_value += 8;
        }
        break;
      case '9':
        if (decimal_place) {
          fractional_value *= 10;
          fractional_value += 9;
          decimal_pos++;
        } else if (exponent) {
          exponential_value *= 10;
          exponential_value += 9;
        } else {
          integral_value *= 10;
          integral_value += 9;
        }
        break;
      default:
        if (*cur_byte == decimal_separator) {
          decimal_place = true;
          exponent = false;
        } else if (*cur_byte == column_separator || *cur_byte == '\n' ||
                   *cur_byte == '\r' || *cur_byte == '\0') {
          /* everything ok? */
          if (decimal_pos > 8) {
            std::cout
                << "ERROR in function ReadAsciiData(): Too many digits on "
                   "decimal place. Maximum is 8 !"
                << std::endl;
            return false;
          }

          /* calculate the final value */
          (*p_data) = integral_value;
          if (decimal_pos) {
            (*p_data) += fractional_value * fractional_factor[decimal_pos];
          }
          if (val_is_negative) {
            (*p_data) *= -1;
          }
          if (exponent) {
            (*p_data) *=
                std::pow(/* n: */ 10,
                         /* n: */ exp_is_negative ? -1 * exponential_value : 1);
          }

          /* Init  */
          val_is_negative = false;
          exp_is_negative = false;
          decimal_place = false;
          exponent = false;
          integral_value = 0;
          fractional_value = 0;
          exponential_value = 1;
          decimal_pos = 0;

          /* Save value */
          p_data++;
          cur_read_value++;

          /* end of the file? */
          if (*cur_byte == '\0') {
            if (cur_read_value < num_values) {
              std::cout
                  << "ERROR in function ReadAsciiData(): End of file reached "
                     "before all values were read !"
                  << std::endl;
              return false;
            }
            return true;
          }

        } else {
          /* Do nothing */
        }
        break;
    }

    /* Consider next byte */
    cur_buffer_pos++;
    cur_byte++;

    /* buffer overrun? */
    if (cur_buffer_pos >= actual_buffer_size) {
      return false;
    }

  } while (cur_read_value < num_values);

  /* Quit */
  return true;
}

} /* namespace muehle */
