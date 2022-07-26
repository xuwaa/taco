#include <iostream>
#include <fstream>
#include <dlfcn.h>
#include <algorithm>
#include <unordered_set>

#include "taco/ir/ir_visitor.h"
#include "codegen_cuda.h"
#include "taco/error.h"
#include "taco/util/strings.h"
#include "taco/util/collections.h"
#include "taco/ir/simplify.h"

using namespace std;

namespace taco {
namespace ir {

// Some helper functions
namespace {

#define CUDA_BLOCK_SIZE 256
// Include stdio.h for printf
// stdlib.h for malloc/realloc
// math.h for sqrt
// MIN preprocessor macro
// This *must* be kept in sync with taco_tensor_t.h
const string cHeaders =
  "#ifndef TACO_C_HEADERS\n"
  "#define TACO_C_HEADERS\n"
  "#include <stdio.h>\n"
  "#include <stdlib.h>\n"
  "#include <stdint.h>\n"
  "#include <math.h>\n"
  "#include <thrust/complex.h>\n"
  "#define TACO_MIN(_a,_b) ((_a) < (_b) ? (_a) : (_b))\n"
  "#define TACO_MAX(_a,_b) ((_a) > (_b) ? (_a) : (_b))\n"
  "#ifndef TACO_TENSOR_T_DEFINED\n"
  "#define TACO_TENSOR_T_DEFINED\n"
  "typedef enum { taco_mode_dense, taco_mode_sparse } taco_mode_t;\n"
  "typedef struct {\n"
  "  int32_t      order;         // tensor order (number of modes)\n"
  "  int32_t*     dimensions;    // tensor dimensions\n"
  "  int32_t      csize;         // component size\n"
  "  int32_t*     mode_ordering; // mode storage ordering\n"
  "  taco_mode_t* mode_types;    // mode storage types\n"
  "  uint8_t***   indices;       // tensor index data (per mode)\n"
  "  uint8_t*     vals;          // tensor values\n"
  "  int32_t      vals_size;     // values array size\n"
  "} taco_tensor_t;\n"
  "#endif\n"
  "#endif\n\n" // // https://stackoverflow.com/questions/14038589/what-is-the-canonical-way-to-check-for-errors-using-the-cuda-runtime-api
  "#define gpuErrchk(ans) { gpuAssert((ans), __FILE__, __LINE__); }\n"
  "inline void gpuAssert(cudaError_t code, const char *file, int line, bool abort=true)\n"
  "{\n"
  "  if (code != cudaSuccess)\n"
  "  {\n"
  "    fprintf(stderr,\"GPUassert: %s %s %d\\n\", cudaGetErrorString(code), file, line);\n"
  "    if (abort) exit(code);\n"
  "  }\n"
  "}\n";

const std::string blue="\033[38;5;67m";
const std::string nc="\033[0m";

// find variables for generating declarations
// also only generates a single var for each GetProperty
class FindVars : public IRVisitor {
public:
  map<Expr, string, ExprCompare> varMap;
  
  // the variables for which we need to add declarations
  map<Expr, string, ExprCompare> varDecls;
  
  // this maps from tensor, property, mode, index to the unique var
  map<tuple<Expr, TensorProperty, int, int>, string> canonicalPropertyVar;
  
  // this is for convenience, recording just the properties unpacked
  // from the output tensor so we can re-save them at the end
  map<tuple<Expr, TensorProperty, int, int>, string> outputProperties;
  
  // TODO: should replace this with an unordered set
  vector<Expr> outputTensors;

  // Stop searching for variables at device functions (used to generate kernel launches)
  bool stopAtDeviceFunction;
  
  // copy inputs and outputs into the map
  FindVars(vector<Expr> inputs, vector<Expr> outputs, bool stopAtDeviceFunction=false)  {
    for (auto v: inputs) {
      auto var = v.as<Var>();
      taco_iassert(var) << "Inputs must be vars in codegen";
      taco_iassert(varMap.count(var) == 0) <<
          "Duplicate input found in codegen";
      varMap[var] = var->name;
    }
    for (auto v: outputs) {
      auto var = v.as<Var>();
      taco_iassert(var) << "Outputs must be vars in codegen";
      taco_iassert(varMap.count(var) == 0) <<
          "Duplicate output found in codegen";

      outputTensors.push_back(v);
      varMap[var] = var->name;
    }
    inVarAssignLHSWithDecl = false;
    FindVars::stopAtDeviceFunction = stopAtDeviceFunction;
  }

protected:
  bool inVarAssignLHSWithDecl;
  using IRVisitor::visit;

  virtual void visit(const For *op) {
    // Don't need to find/initialize loop bounds
    inVarAssignLHSWithDecl = true;
    op->var.accept(this);
    inVarAssignLHSWithDecl = false;
    op->start.accept(this);
    op->end.accept(this);
    op->increment.accept(this);
    if (op->accelerator && stopAtDeviceFunction) {
      return;
    }
    op->contents.accept(this);
  }

  virtual void visit(const Var *op) {
    if (varMap.count(op) == 0) {
      varMap[op] = CodeGen_CUDA::genUniqueName(op->name);
      if (!inVarAssignLHSWithDecl) {
        varDecls[op] = varMap[op];
      }
    }
  }

  virtual void visit(const VarDecl *op) {
    inVarAssignLHSWithDecl = true;
    op->var.accept(this);
    inVarAssignLHSWithDecl = false;
    op->rhs.accept(this);
  }
  
  virtual void visit(const GetProperty *op) {
    if (varMap.count(op) == 0) {
      auto key =
          tuple<Expr,TensorProperty,int,int>(op->tensor,op->property,
                                             (size_t)op->mode,
                                             (size_t)op->index);
      if (canonicalPropertyVar.count(key) > 0) {
        varMap[op] = canonicalPropertyVar[key];
      } else {
        auto unique_name = CodeGen_CUDA::genUniqueName(op->name);
        canonicalPropertyVar[key] = unique_name;
        varMap[op] = unique_name;
        varDecls[op] = unique_name;
        if (util::contains(outputTensors, op->tensor)) {
          outputProperties[key] = unique_name;
        }
      }
    }
  }
};

// Finds all for loops tagged with accelerator and adds statements to deviceFunctions
// Also tracks scope of when device function is called and
// tracks which variables must be passed to function.
class DeviceFunctionCollector : public IRVisitor {
public:
  vector<Stmt> deviceFunctions;
  map<Expr, string, ExprCompare> scopeMap;
  
  // the variables to pass to each device function
  vector<vector<pair<string, Expr>>> functionParameters;
  vector<pair<string, Expr>> currentParameters; // keep as vector so code generation is deterministic
  set<Expr> currentParameterSet;

  vector<pair<string, Expr>> threadIDVars;

  // copy inputs and outputs into the map
  DeviceFunctionCollector(vector<Expr> inputs, vector<Expr> outputs)  {
    inDeviceFunction = false;
    for (auto v: inputs) {
      auto var = v.as<Var>();
      taco_iassert(var) << "Inputs must be vars in codegen";
      taco_iassert(scopeMap.count(var) == 0) <<
          "Duplicate input found in codegen";
      scopeMap[var] = var->name;
    }
    for (auto v: outputs) {
      auto var = v.as<Var>();
      taco_iassert(var) << "Outputs must be vars in codegen";
      taco_iassert(scopeMap.count(var) == 0) <<
          "Duplicate output found in codegen";

      scopeMap[var] = var->name;
    }
    inVarAssignLHSWithDecl = false;
  }

protected:
  bool inVarAssignLHSWithDecl;
  bool inDeviceFunction;
  using IRVisitor::visit;

  virtual void visit(const For *op) {
    // Don't need to find/initialize loop bounds
    inVarAssignLHSWithDecl = true;
    op->var.accept(this);
    inVarAssignLHSWithDecl = false;
    if (op->accelerator) {
      taco_iassert(!inDeviceFunction) << "Nested Device functions not supported";
      deviceFunctions.push_back(op);
      threadIDVars.push_back(pair<string, Expr>(scopeMap[op->var], op->var));
      currentParameters.clear();
      currentParameterSet.clear();
      inDeviceFunction = true;
    }
    op->start.accept(this);
    op->end.accept(this);
    op->increment.accept(this);
    op->contents.accept(this);
    if (op->accelerator) {
      inDeviceFunction = false;
      sort(currentParameters.begin(), currentParameters.end());
      functionParameters.push_back(currentParameters);
    }
  }

  virtual void visit(const Var *op) {
    if (scopeMap.count(op) == 0) {
      string name = CodeGen_CUDA::genUniqueName(op->name);
      if (!inDeviceFunction) {
        scopeMap[op] = name;
      }
    }
    else if (scopeMap.count(op) == 1 && inDeviceFunction && currentParameterSet.count(op) == 0 && op != threadIDVars.back().second) {
      if (!inVarAssignLHSWithDecl) {
        currentParameters.push_back(pair<string, Expr>(scopeMap[op], op));
        currentParameterSet.insert(op);
      }
    }
  }

  virtual void visit(const VarDecl *op) {
    inVarAssignLHSWithDecl = true;
    op->var.accept(this);
    inVarAssignLHSWithDecl = false;
    op->rhs.accept(this);
  }
  
  virtual void visit(const GetProperty *op) {
    if (scopeMap.count(op->tensor) == 0 && !inDeviceFunction) {
      auto key =
          tuple<Expr,TensorProperty,int,int>(op->tensor,op->property,
                                             (size_t)op->mode,
                                             (size_t)op->index);
      auto unique_name = CodeGen_CUDA::genUniqueName(op->name);
      scopeMap[op->tensor] = unique_name;
    }
    else if (scopeMap.count(op->tensor) == 1 && inDeviceFunction && currentParameterSet.count(op->tensor) == 0) {
      currentParameters.push_back(pair<string, Expr>(op->tensor.as<Var>()->name, op->tensor));
      currentParameterSet.insert(op->tensor);
    }
  }
};

// helper to translate from taco type to C++ type
string toCType(Datatype type, bool is_ptr) {
  stringstream ret;
  if (type.isComplex()) {
      if (type.getKind() == Complex64) {
          ret << "thrust::complex<float>";
      }
      else if (type.getKind() == Complex128) {
          ret << "thrust::complex<double>";
      }
      else {
          taco_ierror;
      }
  }
  else {
      ret << type;
  }

  if (is_ptr) {
    ret << "*";
  }
  
  return ret.str();
}

string unpackTensorProperty(string varname, const GetProperty* op,
                            bool is_output_prop) {
  stringstream ret;
  ret << "  ";
  
  auto tensor = op->tensor.as<Var>();
  if (op->property == TensorProperty::Values) {
    // for the values, it's in the last slot
    ret << toCType(tensor->type, true);
    ret << " __restrict__ " << varname << " = (" << toCType(tensor->type, true) << ")(";
    ret << tensor->name << "->vals);\n";
    return ret.str();
  } else if (op->property == TensorProperty::ValuesSize) {
    ret << "int " << varname << " = " << tensor->name << "->vals_size;\n";
    return ret.str();
  }
  
  string tp;
  
  // for a Dense level, nnz is an int
  // for a Fixed level, ptr is an int
  // all others are int*
  if (op->property == TensorProperty::Dimension) {
    tp = "int";
    ret << tp << " " << varname << " = (int)("
        << tensor->name << "->dimensions[" << tensor->name << "->mode_ordering[" 
        << op->mode << "]]);\n";
  } else {
    taco_iassert(op->property == TensorProperty::Indices);
    tp = "int*";
    auto nm = op->index;
    ret << tp << " __restrict__ " << varname << " = ";
    ret << "(int*)(" << tensor->name << "->indices[" << op->mode;
    ret << "][" << nm << "]);\n";
  }
  
  return ret.str();
}

string packTensorProperty(string varname, Expr tnsr, TensorProperty property,
                          int mode, int index) {
  stringstream ret;
  ret << "  ";
  
  auto tensor = tnsr.as<Var>();
  if (property == TensorProperty::Values) {
    ret << tensor->name << "->vals";
    ret << " = (uint8_t*)" << varname << ";\n";
    return ret.str();
  } else if (property == TensorProperty::ValuesSize) {
    ret << tensor->name << "->vals_size = " << varname << ";\n";
    return ret.str();
  }
  
  string tp;
  
  // for a Dense level, nnz is an int
  // for a Fixed level, ptr is an int
  // all others are int*
  if (property == TensorProperty::Dimension) {
    return "";
  } else {
    taco_iassert(property == TensorProperty::Indices);
    tp = "int*";
    auto nm = index;
    ret << tensor->name << "->indices" <<
      "[" << mode << "][" << nm << "] = (uint8_t*)(" << varname
      << ");\n";
  }
  
  return ret.str();
}

  
// helper to print declarations
string printDecls(map<Expr, string, ExprCompare> varMap,
                   vector<Expr> inputs, vector<Expr> outputs) {
  stringstream ret;
  unordered_set<string> propsAlreadyGenerated;
  
  vector<const GetProperty*> sortedProps;
  
  for (auto const& p: varMap) {
    if (p.first.as<GetProperty>())
      sortedProps.push_back(p.first.as<GetProperty>());
  }
  
  // sort the properties in order to generate them in a canonical order
  sort(sortedProps.begin(), sortedProps.end(),
    [&](const GetProperty *a,
        const GetProperty *b) -> bool {
          // first, use a total order of outputs,inputs
          auto a_it = find(outputs.begin(), outputs.end(), a->tensor);
          auto b_it = find(outputs.begin(), outputs.end(), b->tensor);
          auto a_pos = distance(outputs.begin(), a_it);
          auto b_pos = distance(outputs.begin(), b_it);
          if (a_it == outputs.end())
            a_pos += distance(inputs.begin(), find(inputs.begin(), inputs.end(),
                                                   a->tensor));
          if (b_it == outputs.end())
            b_pos += distance(inputs.begin(), find(inputs.begin(), inputs.end(),
                                                   b->tensor));
      
          // if total order is same, have to do more, otherwise we know
          // our answer
          if (a_pos != b_pos)
            return a_pos < b_pos;
      
          // if they're different properties, sort by property
          if (a->property != b->property)
            return a->property < b->property;
      
          // now either the mode gives order, or index #
          if (a->mode != b->mode)
            return a->mode < b->mode;
      
          return a->index < b->index;
       });
  
  for (auto prop: sortedProps) {
    bool isOutputProp = (find(outputs.begin(), outputs.end(),
                          prop->tensor) != outputs.end());
    ret << unpackTensorProperty(varMap[prop], prop, isOutputProp);
    propsAlreadyGenerated.insert(varMap[prop]);
  }

  for (auto varpair: varMap) {
    // make sure it's not an input or output
    if (find(inputs.begin(), inputs.end(), varpair.first) == inputs.end() &&
        find(outputs.begin(), outputs.end(), varpair.first) == outputs.end()) {
      auto var = varpair.first.as<Var>();
      if (var) {
        ret << "  " << toCType(var->type, var->is_ptr);
        ret << " " << varpair.second << ";\n";
      } else {
        taco_iassert(varpair.first.as<GetProperty>());
        // we better have already generated these
        taco_iassert(propsAlreadyGenerated.count(varpair.second));
      }
    }
  }

  return ret.str();
}

// Check if a function has an Allocate node.
// Used to decide if we should print the repack code
class CheckForAlloc : public IRVisitor {
public:
  bool hasAlloc;
  CheckForAlloc() : hasAlloc(false) { }
protected:
  using IRVisitor::visit;
  void visit(const Allocate *op) {
    hasAlloc = true;
  }
};

string printPack(map<tuple<Expr, TensorProperty, int, int>,
                 string> outputProperties,
                 vector<Expr> outputs) {
  stringstream ret;
  vector<tuple<Expr, TensorProperty, int, int>> sortedProps;
  
  for (auto &prop: outputProperties) {
    sortedProps.push_back(prop.first);
  }
  sort(sortedProps.begin(), sortedProps.end(),
       [&](const tuple<Expr, TensorProperty, int, int> &a,
           const tuple<Expr, TensorProperty, int, int> &b) -> bool {
         // first, use a total order of outputs,inputs
         auto a_it = find(outputs.begin(), outputs.end(), get<0>(a));
         auto b_it = find(outputs.begin(), outputs.end(), get<0>(b));
         auto a_pos = distance(outputs.begin(), a_it);
         auto b_pos = distance(outputs.begin(), b_it);
         
         // if total order is same, have to do more, otherwise we know
         // our answer
         if (a_pos != b_pos)
           return a_pos < b_pos;
         
         // if they're different properties, sort by property
         if (get<1>(a) != get<1>(b))
           return get<1>(a) < get<1>(b);
         
         // now either the mode gives order, or index #
         if (get<2>(a) != get<2>(b))
           return get<2>(a) < get<2>(b);
         
         return get<3>(a) < get<3>(b);
       });
  
  for (auto prop: sortedProps) {
    ret << packTensorProperty(outputProperties[prop], get<0>(prop),
      get<1>(prop), get<2>(prop), get<3>(prop));
  }
  return ret.str();
}

// seed the unique names with all C99 keywords
// from: http://en.cppreference.com/w/c/keyword
map<string, int> uniqueNameCounters;

void resetUniqueNameCounters() {
  uniqueNameCounters =
    {{"auto", 0},
     {"break", 0},
     {"case", 0},
     {"char", 0},
     {"const", 0},
     {"continue", 0},
     {"default", 0},
     {"do", 0},
     {"double", 0},
     {"else", 0},
     {"enum", 0},
     {"extern", 0},
     {"float", 0},
     {"for", 0},
     {"goto", 0},
     {"if", 0},
     {"inline", 0},
     {"int", 0},
     {"long", 0},
     {"register", 0},
     {"__restrict__", 0},
     {"return", 0},
     {"short", 0},
     {"signed", 0},
     {"sizeof", 0},
     {"static", 0},
     {"struct", 0},
     {"switch", 0},
     {"typedef", 0},
     {"union", 0},
     {"unsigned", 0},
     {"void", 0},
     {"volatile", 0},
     {"while", 0},
     {"bool", 0},
     {"complex", 0},
     {"imaginary", 0}};
}

string printFuncName(const Function *func) {
  stringstream ret;
  
  ret << "int " << func->name << "(";

  string delimiter = "";
  for (size_t i=0; i<func->outputs.size(); i++) {
    auto var = func->outputs[i].as<Var>();
    taco_iassert(var) << "Unable to convert output " << func->outputs[i]
      << " to Var";
    if (var->is_tensor) {
      ret << delimiter << "taco_tensor_t *" << var->name;
    } else {
      auto tp = toCType(var->type, var->is_ptr);
      ret << delimiter << tp << " " << var->name;
    }
    delimiter = ", ";
  }
  for (size_t i=0; i<func->inputs.size(); i++) {
    auto var = func->inputs[i].as<Var>();
    taco_iassert(var) << "Unable to convert output " << func->inputs[i]
      << " to Var";
    if (var->is_tensor) {
      ret << delimiter << "taco_tensor_t *" << var->name;
    } else {
      auto tp = toCType(var->type, var->is_ptr);
      ret << delimiter << tp << " " << var->name;
    }
    delimiter = ", ";
  }
  
  ret << ")";
  return ret.str();
}
  

} // anonymous namespace

string CodeGen_CUDA::printDeviceFuncName(const vector<pair<string, Expr>> currentParameters, int index) {
  stringstream ret;
  ret << "__global__" << endl;
  ret << "void " << "deviceKernel" << index << "(";

  string delimiter = "";
  for (size_t i=0; i<currentParameters.size(); i++) {
    auto var = currentParameters[i].second.as<Var>();
    taco_iassert(var) << "Unable to convert output " << currentParameters[i].second
      << " to Var";
    string varName = currentParameters[i].first;
    
    if (var->is_tensor) {
      ret << delimiter << "taco_tensor_t *" << varName;
    }
    else {
      auto tp = toCType(var->type, var->is_ptr);
      ret << delimiter << tp << " " << var->name;
    }
    // No non-tensor parameters
    delimiter = ", ";
  }
  ret << ")";
  return ret.str();
}

void CodeGen_CUDA::printThreadIDVariable(pair<string, Expr> threadIDVar, Expr start, Expr increment) {
  auto var = threadIDVar.second.as<Var>();
  taco_iassert(var) << "Unable to convert output " << threadIDVar.second
    << " to Var";
  string varName = threadIDVar.first;
  auto tp = toCType(var->type, var->is_ptr);
  stream << tp << " " << varName << " = ";
  increment = ir::simplify(increment);
  if (!isa<Literal>(increment) || !to<Literal>(increment)->equalsScalar(1)) {
    stream << "(blockIdx.x * blockDim.x + threadIdx.x) * ";
    increment.accept(this);
  }
  else {
    stream << "blockIdx.x * blockDim.x + threadIdx.x";
  }
  Expr expr = ir::simplify(start);
  if (!isa<Literal>(expr) || !to<Literal>(expr)->equalsScalar(0)) {
    stream << " + ";
    expr.accept(this);
  }
  stream << ";\n";
}

void CodeGen_CUDA::printThreadBoundCheck(pair<string, Expr> threadIDVar, Expr end) {
  taco_iassert(threadIDVar.second.as<Var>()) << "Unable to convert output " << threadIDVar.second
    << " to Var";
  string varName = threadIDVar.first;
  end = ir::simplify(end);
  stream << "if (" << varName << " >= ";
  end.accept(this);
  stream << ") {" << "\n";
  indent++;
  doIndent();
  stream << "return;\n";
  indent--;
  doIndent();
  stream << "}" << "\n" << "\n";
}

void CodeGen_CUDA::printDeviceFuncCall(const vector<pair<string, Expr>> currentParameters, int index, Expr start, Expr end, Expr increment) {
  stream << "deviceKernel" << index << "<<<";
  // ensure always rounds up
  Expr loopIterations = Div::make(Add::make(Sub::make(end, start), Sub::make(increment, Literal::make(1, Int()))), increment);
  loopIterations = ir::simplify(loopIterations);
  Expr blockSize = Div::make(Add::make(loopIterations, Literal::make(CUDA_BLOCK_SIZE - 1, Int())), Literal::make(CUDA_BLOCK_SIZE));
  blockSize = ir::simplify(blockSize);
  blockSize.accept(this);
  stream << ", " << CUDA_BLOCK_SIZE << ">>>";
  stream << "(";

  string delimiter = "";
  for (size_t i=0; i<currentParameters.size(); i++) {
    taco_iassert(currentParameters[i].second.as<Var>()) << "Unable to convert output " << currentParameters[i].second
      << " to Var";
    string varName = currentParameters[i].first;
    stream << delimiter << varName;

    delimiter = ", ";
  }
  stream << ");\n";
  doIndent();
  stream << "cudaDeviceSynchronize();\n";
}


string CodeGen_CUDA::genUniqueName(string name) {
  stringstream os;
  os << name;
  if (uniqueNameCounters.count(name) > 0) {
    os << uniqueNameCounters[name]++;
  } else {
    uniqueNameCounters[name] = 0;
  }
  return os.str();
}

CodeGen_CUDA::CodeGen_CUDA(std::ostream &dest, OutputKind outputKind)
    : CodeGen(dest, false, false), out(dest), outputKind(outputKind) {}

CodeGen_CUDA::~CodeGen_CUDA() {}

void CodeGen_CUDA::compile(Stmt stmt, bool isFirst) {
  if (isFirst) {
    // output the headers
    out << cHeaders;
  }
  out << endl;
  stmt = ir::simplify(stmt); // simplify before printing
  // generate code for the Stmt
  stmt.accept(this);
}

void CodeGen_CUDA::printDeviceFunctions(const Function* func) {
  // Collect device functions
    resetUniqueNameCounters();
    DeviceFunctionCollector deviceFunctionCollector(func->inputs, func->outputs);
    func->body.accept(&deviceFunctionCollector);
    deviceFunctions = deviceFunctionCollector.deviceFunctions;
    deviceFunctionParameters = deviceFunctionCollector.functionParameters;

    resetUniqueNameCounters();
    for (size_t i = 0; i < deviceFunctions.size(); i++) {
      const For *forloop = to<For>(deviceFunctions[i]);
      Stmt function = forloop->contents;
      vector<pair<string, Expr>> parameters = deviceFunctionParameters[i];
      // Generate device function header
      doIndent();
      out << printDeviceFuncName(parameters, i);
      out << "{\n";
      indent++;

      // Generate device function code
      resetUniqueNameCounters();
      vector<Expr> inputs;
      for (size_t i = 0; i < parameters.size(); i++) {
        inputs.push_back(parameters[i].second);
      }
      inputs.push_back(deviceFunctionCollector.threadIDVars[i].second);
      FindVars varFinder(inputs, {});
      forloop->accept(&varFinder);
      varMap = varFinder.varMap;

      // Print variable declarations
      out << printDecls(varFinder.varDecls, inputs, {}) << endl;
      doIndent();
      printThreadIDVariable(deviceFunctionCollector.threadIDVars[i], forloop->start, forloop->increment);
      doIndent();
      printThreadBoundCheck(deviceFunctionCollector.threadIDVars[i], forloop->end);

      // output body
      print(function);
      
      // output repack only if we allocated memory
      CheckForAlloc allocChecker;
      func->accept(&allocChecker);
      if (allocChecker.hasAlloc)
        out << endl << printPack(varFinder.outputProperties, func->outputs);      
      indent--;
      doIndent();
      out << "}\n\n";
    }
}

void CodeGen_CUDA::visit(const Function* func) {
  // if generating a header, protect the function declaration with a guard
  if (outputKind == C99Header) {
    out << "#ifndef TACO_GENERATED_" << func->name << "\n";
    out << "#define TACO_GENERATED_" << func->name << "\n";
  }
  else {
    printDeviceFunctions(func);
  }

  // Generate rest of code + calls to device functions

  // output function declaration
  doIndent();
  out << printFuncName(func);
  
  // if we're just generating a header, this is all we need to do
  if (outputKind == C99Header) {
    out << ";\n";
    out << "#endif\n";
    return;
  }

  out << " {\n";

  indent++;

  // find all the vars that are not inputs or outputs and declare them
  resetUniqueNameCounters();
  FindVars varFinder(func->inputs, func->outputs, true);
  func->body.accept(&varFinder);
  varMap = varFinder.varMap;

  // Print variable declarations
  out << printDecls(varFinder.varDecls, func->inputs, func->outputs) << endl;

  // output body
  print(func->body);
  
  // output repack only if we allocated memory
  CheckForAlloc allocChecker;
  func->accept(&allocChecker);
  if (allocChecker.hasAlloc)
    out << endl << printPack(varFinder.outputProperties, func->outputs);
  
  doIndent();
  out << "return 0;\n";
  indent--;

  doIndent();
  out << "}\n";
}

// For Vars, we replace their names with the generated name,
// since we match by reference (not name)
void CodeGen_CUDA::visit(const Var* op) {
  taco_iassert(varMap.count(op) > 0) <<
      "Var " << op->name << " not found in varMap";
  out << varMap[op];
}

static string genVectorizePragma(int width) {
  stringstream ret;
  ret << "#pragma clang loop interleave(enable) ";
  if (!width)
    ret << "vectorize(enable)";
  else
    ret << "vectorize_width(" << width << ")";
  
  return ret.str();
}

static string getParallelizePragma(LoopKind kind) {
  stringstream ret;
  ret << "#pragma omp parallel for";
  if (kind == LoopKind::Dynamic) {
    ret << " schedule(dynamic, 16)";
  }
  return ret.str();
}

// The next two need to output the correct pragmas depending
// on the loop kind (Serial, Static, Dynamic, Vectorized)
//
// Docs for vectorization pragmas:
// http://clang.llvm.org/docs/LanguageExtensions.html#extensions-for-loop-hint-optimizations
void CodeGen_CUDA::visit(const For* op) {
  for (size_t i = 0; i < deviceFunctions.size(); i++) {
    auto dFunction = deviceFunctions[i].as<For>();
    assert(dFunction);
    if (op == dFunction) {
      // Generate kernel launch
      doIndent();
      printDeviceFuncCall(deviceFunctionParameters[i], i, op->start, op->end, op->increment);
      return;
    }
  }

  switch (op->kind) {
    case LoopKind::Vectorized:
      doIndent();
      out << genVectorizePragma(op->vec_width);
      out << "\n";
      break;
    case LoopKind::Static:
    case LoopKind::Dynamic:
      doIndent();
      out << getParallelizePragma(op->kind);
      out << "\n";
    default:
      break;
  }

  IRPrinter::visit(op);
}

void CodeGen_CUDA::visit(const While* op) {
  // it's not clear from documentation that clang will vectorize
  // while loops
  // however, we'll output the pragmas anyway
  if (op->kind == LoopKind::Vectorized) {
    doIndent();
    out << genVectorizePragma(op->vec_width);
    out << "\n";
  }
  
  IRPrinter::visit(op);
}

void CodeGen_CUDA::visit(const GetProperty* op) {
  taco_iassert(varMap.count(op) > 0) <<
      "Property of " << op->tensor << " not found in varMap";
  out << varMap[op];
}

void CodeGen_CUDA::visit(const Min* op) {
  if (op->operands.size() == 1) {
    op->operands[0].accept(this);
    return;
  }
  for (size_t i=0; i<op->operands.size()-1; i++) {
    stream << "TACO_MIN(";
    op->operands[i].accept(this);
    stream << ",";
  }
  op->operands.back().accept(this);
  for (size_t i=0; i<op->operands.size()-1; i++) {
    stream << ")";
  }
}

void CodeGen_CUDA::visit(const Max* op) {
  stream << "TACO_MAX(";
  op->a.accept(this);
  stream << ", ";
  op->b.accept(this);
  stream << ")";
}

void CodeGen_CUDA::visit(const Allocate* op) {
  string elementType = toCType(op->var.type(), false);
  string variable_name;
  if (op->is_realloc) {
    // cuda doesn't have realloc
    // void * tmp_realloc_ptr;
    // gpuErrchk(cudaMallocManaged((void**)& tmp_realloc_ptr, new_size);
    // memcpy(tmp_realloc_ptr, var, TACO_MIN(old_size, new_size));
    // cudaFree(var);
    // var = tmp_realloc_ptr;

    doIndent();
    variable_name = genUniqueName("tmp_realloc_ptr");
    stream << "void * " << variable_name << ";" << endl;
  }

  doIndent();
  stream << "gpuErrchk(cudaMallocManaged((void**)&";
  if (op->is_realloc) {
    stream << variable_name;
  }
  else {
    op->var.accept(this);
  }
  stream << ", ";
  stream << "sizeof(" << elementType << ")";
  stream << " * ";
  op->num_elements.accept(this);
  stream << "));" << endl;

  if(op->is_realloc) {
    doIndent();
    stream << "memcpy(" << variable_name << ", ";
    op->var.accept(this);
    stream << ", ";
    stream << "TACO_MIN(";
    op->old_elements.accept(this);
    stream << ", ";
    op->num_elements.accept(this);
    stream << ")";
    stream << " * sizeof(" << elementType << "));" << endl;

    doIndent();
    stream << "cudaFree(";
    op->var.accept(this);
    stream << ");" << endl;

    doIndent();
    op->var.accept(this);
    stream << " = (" << toCType(op->var.type(), true) << ") " << variable_name << ";" << endl;    
  }

}

void CodeGen_CUDA::visit(const Sqrt* op) {
  taco_tassert(op->type.isFloat() && op->type.getNumBits() == 64) <<
      "Codegen doesn't currently support non-double sqrt";
  stream << "sqrt(";
  op->a.accept(this);
  stream << ")";
}

void CodeGen_CUDA::visit(const VarDecl* op) {
    doIndent();
    stream << keywordString(util::toString(toCType(op->var.type(), false))) << " ";
    string varName = varNameGenerator.getUniqueName(util::toString(op->var));
    varNames.insert({op->var, varName});
    op->var.accept(this);
    parentPrecedence = Precedence::TOP;
    stream << " = ";
    op->rhs.accept(this);
    stream << ";";
    stream << endl;
}

// Need to handle some binary ops so that we can add the necessary casts if complex
// Because c++ does not properly handle double * std::complex<float> or std::complex<float> * std::complex<double>
void CodeGen_CUDA::printBinCastedOp(Expr a, Expr b, string op, Precedence precedence) {
  bool parenthesize = precedence > parentPrecedence;
  if (parenthesize) {
    stream << "(";
  }
  parentPrecedence = precedence;
  Datatype mType = max_type(a.type(), b.type());
  if (mType.isComplex() && mType != a.type()) {
    stream << "(" << toCType(mType, false) << ") ";
  }
  a.accept(this);
  stream << " " << op << " ";
  parentPrecedence = precedence;
  if (mType.isComplex() && mType != b.type()) {
    stream << "(" << toCType(mType, false) << ") ";
  }
  b.accept(this);
  if (parenthesize) {
    stream << ")";
  }
}

void CodeGen_CUDA::visit(const Add* op) {
  printBinCastedOp(op->a, op->b, "+", Precedence::ADD);
}

void CodeGen_CUDA::visit(const Sub* op) {
  printBinCastedOp(op->a, op->b, "-", Precedence::SUB);
}

void CodeGen_CUDA::visit(const Mul* op) {
  printBinCastedOp(op->a, op->b, "*", Precedence::MUL);
}

void CodeGen_CUDA::visit(const Div* op) {
  printBinCastedOp(op->a, op->b, "/", Precedence::DIV);
}

void CodeGen_CUDA::visit(const Literal* op) {
  if (op->type.isComplex()) {
    if (color) {
      stream << blue ;
    }

    if(op->type.getKind() == Complex64) {
      std::complex<float> val = op->getValue<std::complex<float>>();
      stream << "thrust::complex<float>(" << val.real() << ", " << val.imag() << ")";
    }
    else if(op->type.getKind() == Complex128) {
      std::complex<double> val = op->getValue<std::complex<double>>();
      stream << "thrust::complex<double>(" << val.real() << ", " << val.imag() << ")";
    }
    else {
      taco_ierror << "Undefined type in IR";
    }

    if (color) {
      stream << nc;
    }
  }
  else {
    IRPrinter::visit(op);
  }
}
  
void CodeGen_CUDA::generateShim(const Stmt& func, stringstream &ret) {
  const Function *funcPtr = func.as<Function>();
  ret << "extern \"C\" {\n";
  ret << "  int _shim_" << funcPtr->name << "(void** parameterPack);\n";
  ret << "}\n\n";

  ret << "int _shim_" << funcPtr->name << "(void** parameterPack) {\n";
  ret << "  return " << funcPtr->name << "(";
  
  size_t i=0;
  string delimiter = "";
  for (auto output : funcPtr->outputs) {
    auto var = output.as<Var>();
    auto cast_type = var->is_tensor ? "taco_tensor_t*"
    : toCType(var->type, var->is_ptr);
    
    ret << delimiter << "(" << cast_type << ")(parameterPack[" << i++ << "])";
    delimiter = ", ";
  }
  for (auto input : funcPtr->inputs) {
    auto var = input.as<Var>();
    auto cast_type = var->is_tensor ? "taco_tensor_t*"
    : toCType(var->type, var->is_ptr);
    ret << delimiter << "(" << cast_type << ")(parameterPack[" << i++ << "])";
    delimiter = ", ";
  }
  ret << ");\n";
  ret << "}\n";
}

}}
