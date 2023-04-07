
#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "util.hpp"

enum class Opcode {
  Add,
  Sub,
  Mult,
  Multu,
  Div,
  Divu,
  Mfhi,
  Mflo,
  Lis,
  Lw,
  Sw,
  Slt,
  Sltu,
  Beq,
  Bne,
  Jr,
  Jalr,
  Word,
  Label,
  Import,
  Comment,
  NumOpcodes,
};

struct MIPSInstruction {
  Opcode opcode;
  int s, t, d;
  int32_t i;

  bool has_label;
  std::string string_value;
  std::string comment_value;

  static inline const char *opcode_to_string(const Opcode op) {
    switch (op) {
    case Opcode::Add:
      return "add";
    case Opcode::Sub:
      return "sub";
    case Opcode::Mult:
      return "mult";
    case Opcode::Multu:
      return "multu";
    case Opcode::Div:
      return "div";
    case Opcode::Divu:
      return "divu";
    case Opcode::Mfhi:
      return "mfhi";
    case Opcode::Mflo:
      return "mflo";
    case Opcode::Lis:
      return "lis";
    case Opcode::Lw:
      return "lw";
    case Opcode::Sw:
      return "sw";
    case Opcode::Slt:
      return "slt";
    case Opcode::Sltu:
      return "sltu";
    case Opcode::Beq:
      return "beq";
    case Opcode::Bne:
      return "bne";
    case Opcode::Jr:
      return "jr";
    case Opcode::Jalr:
      return "jalr";
    default:
      return "";
    }
  }

private:
  MIPSInstruction(Opcode opcode, int s, int t, int d, int32_t i,
                  bool has_label = false, const std::string &label_name = "")
      : opcode(opcode), s(s), t(t), d(d), i(i), has_label(has_label),
        string_value(label_name) {}
  MIPSInstruction(const std::string &label_name)
      : opcode(Opcode::Label), s(0), t(0), d(0), i(0), has_label(true),
        string_value(label_name) {}

public:
  static MIPSInstruction add(int d, int s, int t) {
    return MIPSInstruction(Opcode::Add, s, t, d, 0);
  }
  static MIPSInstruction sub(int d, int s, int t) {
    return MIPSInstruction(Opcode::Sub, s, t, d, 0);
  }
  static MIPSInstruction mult(int s, int t) {
    return MIPSInstruction(Opcode::Mult, s, t, 0, 0);
  }
  static MIPSInstruction multu(int s, int t) {
    return MIPSInstruction(Opcode::Multu, s, t, 0, 0);
  }
  static MIPSInstruction div(int s, int t) {
    return MIPSInstruction(Opcode::Div, s, t, 0, 0);
  }
  static MIPSInstruction divu(int s, int t) {
    return MIPSInstruction(Opcode::Divu, s, t, 0, 0);
  }
  static MIPSInstruction mfhi(int d) {
    return MIPSInstruction(Opcode::Mfhi, 0, 0, d, 0);
  }
  static MIPSInstruction mflo(int d) {
    return MIPSInstruction(Opcode::Mflo, 0, 0, d, 0);
  }
  static MIPSInstruction lis(int d) {
    return MIPSInstruction(Opcode::Lis, 0, 0, d, 0);
  }
  static MIPSInstruction lw(int t, int32_t i, int s) {
    return MIPSInstruction(Opcode::Lw, s, t, 0, i);
  }
  static MIPSInstruction sw(int t, int32_t i, int s) {
    return MIPSInstruction(Opcode::Sw, s, t, 0, i);
  }
  static MIPSInstruction slt(int d, int s, int t) {
    return MIPSInstruction(Opcode::Slt, s, t, d, 0);
  }
  static MIPSInstruction sltu(int d, int s, int t) {
    return MIPSInstruction(Opcode::Sltu, s, t, d, 0);
  }
  static MIPSInstruction beq(int s, int t, int i) {
    return MIPSInstruction(Opcode::Beq, s, t, 0, i);
  }
  static MIPSInstruction beq(int s, int t, const std::string &label) {
    return MIPSInstruction(Opcode::Beq, s, t, 0, 0, true, label);
  }
  static MIPSInstruction bne(int s, int t, int i) {
    return MIPSInstruction(Opcode::Bne, s, t, 0, i);
  }
  static MIPSInstruction bne(int s, int t, const std::string &label) {
    return MIPSInstruction(Opcode::Bne, s, t, 0, 0, true, label);
  }
  static MIPSInstruction jr(int s) {
    return MIPSInstruction(Opcode::Jr, s, 0, 0, 0);
  }
  static MIPSInstruction jalr(int s) {
    return MIPSInstruction(Opcode::Jalr, s, 0, 0, 0);
  }
  static MIPSInstruction word(int32_t i) {
    return MIPSInstruction(Opcode::Word, 0, 0, 0, i);
  }
  static MIPSInstruction word(const std::string &label) {
    return MIPSInstruction(Opcode::Word, 0, 0, 0, 0, true, label);
  }
  static MIPSInstruction label(const std::string &name) {
    return MIPSInstruction(name);
  }
  static MIPSInstruction import(const std::string &value) {
    return MIPSInstruction(Opcode::Import, 0, 0, 0, 0, true, value);
  }
  static MIPSInstruction comment(const std::string &value) {
    return MIPSInstruction(Opcode::Comment, 0, 0, 0, 0, true, value);
  }

  std::string to_string() const {
    std::stringstream ss;
    const std::string name = opcode_to_string(opcode);
    const int instruction_width = 32;
    switch (opcode) {
    case Opcode::Add:
    case Opcode::Sub:
    case Opcode::Slt:
    case Opcode::Sltu:
      ss << name << " $" << d << ", $" << s << ", $" << t;
      break;
    case Opcode::Mult:
    case Opcode::Multu:
    case Opcode::Div:
    case Opcode::Divu:
      ss << name << " $" << s << ", $" << t;
      break;
    case Opcode::Mfhi:
    case Opcode::Mflo:
    case Opcode::Lis:
      ss << name << " $" << d;
      break;
    case Opcode::Lw:
    case Opcode::Sw:
      ss << name << " $" << t << ", " << i << "($" << s << ")";
      break;
    case Opcode::Beq:
    case Opcode::Bne:
      if (has_label) {
        ss << name << " $" << s << ", $" << t << ", " << string_value;
      } else {
        ss << name << " $" << s << ", $" << t << ", " << i;
      }
      break;
    case Opcode::Jr:
    case Opcode::Jalr:
      ss << name << " $" << s;
      break;
    case Opcode::Word:
      if (has_label) {
        ss << ".word " << string_value;
      } else {
        ss << ".word " << i;
      }
      break;
    case Opcode::Label:
      ss << string_value << ":";
      break;
    case Opcode::Import:
      ss << ".import " << string_value;
      break;
    case Opcode::Comment:
      ss << std::string(instruction_width, ' ') << "; " << string_value;
      break;
    default:
      runtime_assert(false, "Invalid opcode");
    }

    
    const int padding =
        std::max(0, instruction_width - static_cast<int>(ss.str().size()));
    if (comment_value != "")
      ss << std::string(padding, ' ') << "; " << comment_value;

    return ss.str();
  }
};
