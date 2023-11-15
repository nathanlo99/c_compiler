
#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <optional>
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

enum class Reg {
  R0,
  R1,
  R2,
  R3,
  R4,
  R5,
  R6,
  R7,
  R8,
  R9,
  R10,
  R11,
  R12,
  R13,
  R14,
  R15,
  R16,
  R17,
  R18,
  R19,
  R20,
  R21,
  R22,
  R23,
  R24,
  R25,
  R26,
  R27,
  R28,
  R29,
  R30,
  R31,
  // Aliases
  FP = R29,
  SP = R30,
};

template <> struct fmt::formatter<Reg> : formatter<string_view> {
  // parse is inherited from formatter<string_view>.
  template <typename FormatContext>
  auto format(Reg reg, FormatContext &ctx) const {
    string_view name = "???";
    switch (reg) {
    case Reg::R0:
      name = "$0";
      break;
    case Reg::R1:
      name = "$1";
      break;
    case Reg::R2:
      name = "$2";
      break;
    case Reg::R3:
      name = "$3";
      break;
    case Reg::R4:
      name = "$4";
      break;
    case Reg::R5:
      name = "$5";
      break;
    case Reg::R6:
      name = "$6";
      break;
    case Reg::R7:
      name = "$7";
      break;
    case Reg::R8:
      name = "$8";
      break;
    case Reg::R9:
      name = "$9";
      break;
    case Reg::R10:
      name = "$10";
      break;
    case Reg::R11:
      name = "$11";
      break;
    case Reg::R12:
      name = "$12";
      break;
    case Reg::R13:
      name = "$13";
      break;
    case Reg::R14:
      name = "$14";
      break;
    case Reg::R15:
      name = "$15";
      break;
    case Reg::R16:
      name = "$16";
      break;
    case Reg::R17:
      name = "$17";
      break;
    case Reg::R18:
      name = "$18";
      break;
    case Reg::R19:
      name = "$19";
      break;
    case Reg::R20:
      name = "$20";
      break;
    case Reg::R21:
      name = "$21";
      break;
    case Reg::R22:
      name = "$22";
      break;
    case Reg::R23:
      name = "$23";
      break;
    case Reg::R24:
      name = "$24";
      break;
    case Reg::R25:
      name = "$25";
      break;
    case Reg::R26:
      name = "$26";
      break;
    case Reg::R27:
      name = "$27";
      break;
    case Reg::R28:
      name = "$28";
      break;
    case Reg::R29:
      name = "$29";
      break;
    case Reg::R30:
      name = "$30";
      break;
    case Reg::R31:
      name = "$31";
      break;
    }
    return formatter<string_view>::format(name, ctx);
  }
};

inline std::ostream &operator<<(std::ostream &os, const Reg &reg) {
  return os << fmt::format("{}", reg);
}

struct MIPSInstruction {
  Opcode opcode;
  Reg s, t, d;
  int64_t i;

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
  static std::string make_label(const std::string &label) {
    std::stringstream ss;
    bool capitalize_next = false;
    for (const char ch : label) {
      if (ch == '_') {
        capitalize_next = true;
        continue;
      } else if (capitalize_next && 'a' <= ch && ch <= 'z') {
        ss << static_cast<char>(ch + 'A' - 'a');
      } else {
        ss << ch;
      }
      capitalize_next = false;
    }
    return ss.str();
  }

  MIPSInstruction(Opcode opcode, Reg s, Reg t, Reg d, int64_t i,
                  bool has_label = false, const std::string &label_name = "")
      : opcode(opcode), s(s), t(t), d(d), i(i), has_label(has_label),
        string_value(make_label(label_name)) {}
  MIPSInstruction(const std::string &label_name)
      : opcode(Opcode::Label), s(Reg::R0), t(Reg::R0), d(Reg::R0), i(0),
        has_label(true), string_value(make_label(label_name)) {}

public:
  static MIPSInstruction add(Reg d, Reg s, Reg t) {
    return MIPSInstruction(Opcode::Add, s, t, d, 0);
  }
  static MIPSInstruction sub(Reg d, Reg s, Reg t) {
    return MIPSInstruction(Opcode::Sub, s, t, d, 0);
  }
  static MIPSInstruction mult(Reg s, Reg t) {
    return MIPSInstruction(Opcode::Mult, s, t, Reg::R0, 0);
  }
  static MIPSInstruction multu(Reg s, Reg t) {
    return MIPSInstruction(Opcode::Multu, s, t, Reg::R0, 0);
  }
  static MIPSInstruction div(Reg s, Reg t) {
    return MIPSInstruction(Opcode::Div, s, t, Reg::R0, 0);
  }
  static MIPSInstruction divu(Reg s, Reg t) {
    return MIPSInstruction(Opcode::Divu, s, t, Reg::R0, 0);
  }
  static MIPSInstruction mfhi(Reg d) {
    return MIPSInstruction(Opcode::Mfhi, Reg::R0, Reg::R0, d, 0);
  }
  static MIPSInstruction mflo(Reg d) {
    return MIPSInstruction(Opcode::Mflo, Reg::R0, Reg::R0, d, 0);
  }
  static MIPSInstruction lis(Reg d) {
    return MIPSInstruction(Opcode::Lis, Reg::R0, Reg::R0, d, 0);
  }
  static MIPSInstruction lw(Reg t, int32_t i, Reg s) {
    return MIPSInstruction(Opcode::Lw, s, t, Reg::R0, i);
  }
  static MIPSInstruction sw(Reg t, int32_t i, Reg s) {
    return MIPSInstruction(Opcode::Sw, s, t, Reg::R0, i);
  }
  static MIPSInstruction slt(Reg d, Reg s, Reg t) {
    return MIPSInstruction(Opcode::Slt, s, t, d, 0);
  }
  static MIPSInstruction sltu(Reg d, Reg s, Reg t) {
    return MIPSInstruction(Opcode::Sltu, s, t, d, 0);
  }
  static MIPSInstruction beq(Reg s, Reg t, int64_t i) {
    debug_assert(false, "WARN: Using beq with an immediate value impedes "
                        "peephole optimizations");
    return MIPSInstruction(Opcode::Beq, s, t, Reg::R0, i);
  }
  static MIPSInstruction beq(Reg s, Reg t, const std::string &label) {
    return MIPSInstruction(Opcode::Beq, s, t, Reg::R0, 0, true, label);
  }
  static MIPSInstruction bne(Reg s, Reg t, int64_t i) {
    debug_assert(false, "WARN: Using bne with an immediate value impedes "
                        "peephole optimizations");
    return MIPSInstruction(Opcode::Bne, s, t, Reg::R0, i);
  }
  static MIPSInstruction bne(Reg s, Reg t, const std::string &label) {
    return MIPSInstruction(Opcode::Bne, s, t, Reg::R0, 0, true, label);
  }
  static MIPSInstruction jr(Reg s) {
    return MIPSInstruction(Opcode::Jr, s, Reg::R0, Reg::R0, 0);
  }
  static MIPSInstruction jalr(Reg s) {
    return MIPSInstruction(Opcode::Jalr, s, Reg::R0, Reg::R0, 0);
  }
  static MIPSInstruction word(int64_t i) {
    return MIPSInstruction(Opcode::Word, Reg::R0, Reg::R0, Reg::R0, i);
  }
  static MIPSInstruction word(const std::string &label) {
    return MIPSInstruction(Opcode::Word, Reg::R0, Reg::R0, Reg::R0, 0, true,
                           label);
  }
  static MIPSInstruction label(const std::string &name) {
    return MIPSInstruction(name);
  }
  static MIPSInstruction import_module(const std::string &value) {
    return MIPSInstruction(Opcode::Import, Reg::R0, Reg::R0, Reg::R0, 0, true,
                           value);
  }
  static MIPSInstruction comment(const std::string &value) {
    auto result = MIPSInstruction(Opcode::Comment, Reg::R0, Reg::R0, Reg::R0, 0,
                                  true, "");
    result.comment_value = value;
    return result;
  }

  bool is_jump() const {
    return opcode == Opcode::Jr || opcode == Opcode::Jalr ||
           opcode == Opcode::Beq || opcode == Opcode::Bne;
  }

  bool substitute_arguments(const Reg from, const Reg to) {
    if (from == to)
      return false;
    bool changed = false;
    // If [from] appears as an argument, replace it with [to].
    switch (opcode) {
    case Opcode::Add:
    case Opcode::Sub:
    case Opcode::Mult:
    case Opcode::Multu:
    case Opcode::Div:
    case Opcode::Divu:
    case Opcode::Slt:
    case Opcode::Sltu:
    case Opcode::Beq:
    case Opcode::Bne:
    case Opcode::Sw: {
      if (s == from) {
        s = to;
        changed = true;
      }
      if (t == from) {
        t = to;
        changed = true;
      }
    } break;
    case Opcode::Lw: {
      if (s == from) {
        s = to;
        changed = true;
      }
    } break;
    default:
      break;
    }
    if (changed)
      comment_value += fmt::format(" (replaced {} with {})", from, to);
    return changed;
  }

  std::unordered_set<Reg> read_registers() const {
    std::unordered_set<Reg> result;
    switch (opcode) {
    case Opcode::Add:
    case Opcode::Sub:
    case Opcode::Mult:
    case Opcode::Multu:
    case Opcode::Div:
    case Opcode::Divu:
    case Opcode::Slt:
    case Opcode::Sltu:
    case Opcode::Beq:
    case Opcode::Bne:
    case Opcode::Sw:
      return {s, t};
    case Opcode::Mfhi:
    case Opcode::Mflo:
    case Opcode::Lis:
    case Opcode::Word:
    case Opcode::Label:
    case Opcode::Import:
    case Opcode::Comment:
      return {};
    case Opcode::Lw:
    case Opcode::Jr:
    case Opcode::Jalr:
      return {s};
    default:
      unreachable("Unknown opcode");
    }
    return {};
  }

  std::optional<Reg> written_register() const {
    switch (opcode) {
    case Opcode::Add:
    case Opcode::Sub:
    case Opcode::Mfhi:
    case Opcode::Mflo:
    case Opcode::Lis:
    case Opcode::Slt:
    case Opcode::Sltu:
      return d;
    case Opcode::Lw:
      return t;
    case Opcode::Mult:
    case Opcode::Multu:
    case Opcode::Div:
    case Opcode::Divu:
    case Opcode::Sw:
    case Opcode::Beq:
    case Opcode::Bne:
    case Opcode::Jr:
    case Opcode::Jalr:
    case Opcode::Word:
    case Opcode::Label:
    case Opcode::Import:
    case Opcode::Comment:
      return std::nullopt;
    default:
      unreachable("Unknown opcode");
    }
    return std::nullopt;
  }

  std::string to_string() const {
    std::ostringstream ss;
    const std::string name = opcode_to_string(opcode);
    constexpr int instruction_width = 32;
    switch (opcode) {
    case Opcode::Add:
    case Opcode::Sub:
    case Opcode::Slt:
    case Opcode::Sltu:
      ss << fmt::format("{} {}, {}, {}", name, d, s, t);
      break;
    case Opcode::Mult:
    case Opcode::Multu:
    case Opcode::Div:
    case Opcode::Divu:
      ss << fmt::format("{} {}, {}", name, s, t);
      break;
    case Opcode::Mfhi:
    case Opcode::Mflo:
    case Opcode::Lis:
      ss << fmt::format("{} {}", name, d);
      break;
    case Opcode::Lw:
    case Opcode::Sw:
      ss << fmt::format("{} {}, {}({})", name, t, i, s);
      break;
    case Opcode::Beq:
    case Opcode::Bne:
      if (has_label) {
        ss << fmt::format("{} {}, {}, {}", name, s, t, string_value);
      } else {
        ss << fmt::format("{} {}, {}, {}", name, s, t, i);
      }
      break;
    case Opcode::Jr:
    case Opcode::Jalr:
      ss << fmt::format("{} {}", name, s);
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
      break;
    default:
      debug_assert(false, "Invalid opcode");
    }

    const int padding =
        std::max(0, instruction_width - static_cast<int>(ss.str().size()));
    if (opcode == Opcode::Comment || comment_value != "")
      ss << std::string(padding, ' ') << "; " << comment_value;

    return ss.str();
  }

  bool operator==(const MIPSInstruction &other) const {
    return opcode == other.opcode && s == other.s && t == other.t &&
           d == other.d && i == other.i;
  }
};
