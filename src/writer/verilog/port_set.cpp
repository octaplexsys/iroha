#include "writer/verilog/port_set.h"

#include "writer/verilog/port.h"
#include "iroha/stl_util.h"

#include <ctype.h>

static const string kInput = "input";
static const string kOutput = "output";

namespace iroha {
namespace writer {
namespace verilog {

PortSet::~PortSet() {
  STLDeleteValues(&ports_);
}

Port *PortSet::AddPort(const string &name, enum Port::PortType type,
		       int width) {
  return AddPrefixedPort("", name, type, width);
}

Port *PortSet::AddPrefixedPort(const string &prefix, const string &name,
			       enum Port::PortType type, int width) {
  Port *p = new Port(prefix, name, type, width);
  ports_.push_back(p);
  if (type == Port::INPUT_CLK) {
    clk_ = name;
  }
  if (type == Port::INPUT_RESET) {
    reset_ = name;
  }
  return p;
}

const string &PortSet::GetClk() const {
  return clk_;
}

const string &PortSet::GetReset() const {
  return reset_;
}

void PortSet::Output(enum OutputType type, ostream &os) const {
  OutputWithFlavor(type, "", os);
}

void PortSet::OutputWithFlavor(enum OutputType type, const string &flavor,
			       ostream &os) const {
  bool is_first = true;
  for (Port *p : ports_) {
    OutputPort(p, type, is_first, flavor, os);
    is_first = false;
  }
}

void PortSet::OutputPort(Port *p, enum OutputType type, bool is_first,
			 const string &flavor, ostream &os) const {
  if (type == FIXED_VALUE_ASSIGN) {
    OutputFixedValueAssign(p, os);
  }
  if (type == PORT_CONNECTION_DATA) {
    OutputConnectionData(p, is_first, os);
  }
  if (type == PORT_MODULE_HEAD || type == PORT_MODULE_HEAD_DIRECTION) {
    OutputModuleHead(p, type, is_first, os);
  }
  if (type == PORT_CONNECTION || type == PORT_CONNECTION_TEMPLATE) {
    OutputConnection(p, type, is_first, flavor, os);
  }
  if (type == AXI_USER_ASSIGN_TEMPLATE) {
    if (IsVivadoAxiFlavor(flavor)) {
      if (IsAxiUserOutput(p)) {
	os << " assign " << VivadoPortWireName(p) << " = 0;";
      }
    }
  }
}

void PortSet::OutputConnectionData(Port *p, bool is_first, ostream &os) const {
  if (!is_first) {
    os << ",";
  }
  os << DirectionPort(p->GetType()) << ":" << p->GetWidth() << ":"
     << p->GetPrefix() << ":" << p->GetName();
}

void PortSet::OutputConnection(Port *p, enum OutputType type, bool is_first,
			       const string &flavor, ostream &os) const {
  if (!is_first) {
    os << ", ";
  }
  os << "." << p->GetFullName() << "(";
  if (type == PORT_CONNECTION) {
    os << p->GetFullName();
  }
  if (type == PORT_CONNECTION_TEMPLATE) {
    if (p->GetIsAxi() && IsVivadoAxiFlavor(flavor)) {
      if (IsAxiUserInput(p)) {
	int w = p->GetWidth();
	if (w == 0) {
	  w = 1;
	}
	os << w << "'b0";
      } else if (IsAxiAddrInput(p)) {
	int w = 32 - p->GetAxiAddrWidth();
	os << "{" << w << "'b0, " << VivadoPortWireName(p) << "}";
      } else if (IsAxiUserOutput(p)) {
	// outputs nothing.
      } else {
	os << VivadoPortWireName(p);
      }
    }
  }
  os << ")";
}

void PortSet::OutputModuleHead(Port *p, enum OutputType type, bool is_first,
			       ostream &os) const {
  if (is_first) {
    os << "\n";
  } else {
    os << ",\n";
  }
  Port::PortType port_type = p->GetType();
  os << "  " << DirectionPort(port_type);
  // omitted for PORT_MODULE_HEAD_DIRECTION
  if (type == PORT_MODULE_HEAD && port_type == Port::OUTPUT) {
    os << " reg";
  }
  if (p->GetWidth() > 0) {
    os << " [" << (p->GetWidth() - 1) << ":0]";
  }
  os << " " << p->GetFullName();
}

void PortSet::OutputFixedValueAssign(Port *p, ostream &os) const {
  int v = p->GetFixedValue();
  if (v < 0) {
    return;
  }
  os << "  assign " << p->GetFullName() << " = " << v << ";\n";
}

const string &PortSet::DirectionPort(Port::PortType type) {
  if (type == Port::INPUT || type == Port::INPUT_CLK ||
      type == Port::INPUT_RESET) {
    return kInput;
  } else {
    return kOutput;
  }
}

string PortSet::VivadoPortWireName(Port *p) const {
  string n = p->GetName();
  // To lower.
  char buf[n.size() + 1];
  buf[n.size()] = 0;
  for (int i = 0; i < n.size(); ++i) {
    char c = (n.c_str())[i];
    buf[i] = tolower(c);
  }
  return p->GetPrefix() + "axi_" + string(buf);
}

bool PortSet::IsVivadoAxiFlavor(const string &flavor) {
  return (flavor == "vivado-axi");
}

bool PortSet::IsAxiUserOutput(Port *p) {
  return (IsAxiUser(p) && DirectionPort(p->GetType()) == kOutput);
}

bool PortSet::IsAxiUserInput(Port *p) {
  return (IsAxiUser(p) && DirectionPort(p->GetType()) == kInput);
}

bool PortSet::IsAxiUser(Port *p) {
  if (!p->GetIsAxi()) {
    return false;
  }
  const string &name = p->GetName();
  if (string(name.c_str() + name.size() - 4) == "USER") {
    return true;
  }
  return false;
}

bool PortSet::IsAxiAddrInput(Port *p) {
  if (!p->GetIsAxi()) {
    return false;
  }
  if (DirectionPort(p->GetType()) != kInput) {
    return false;
  }
  const string &name = p->GetName();
  if (string(name.c_str() + name.size() - 4) == "ADDR") {
    return true;
  }
  return false;
}

}  // namespace verilog
}  // namespace writer
}  // namespace iroha
