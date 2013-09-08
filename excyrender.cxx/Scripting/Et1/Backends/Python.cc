// (C) 2013 Sebastian Mach (1983), this file is published under the terms of the
// GNU General Public License, Version 3 (a.k.a. GPLv3).
// See COPYING in the root-folder of the excygen project folder.

#include "Python.hh"
#include "Python.h"
#include "../UnitTesting.hh"
#include <stack>


//--------------------------------------------------------------------------------------------------
TEST_CASE( "Et1/Backends/Python", "Python backend" ) {

    using namespace excyrender::Nature::Et1::Backends;
    return;

    std::string py = "let f(x) = x*2.0, z=let foo(frob)=frob+1.0 in foo(1.0) < f(3.0) in z";
    std::string c = to_python(py);

    std::cerr << "--------------------\n";
    std::cerr << py << std::endl;
    std::cerr << "--------------------\n";
    std::cerr << c << std::endl;
    std::cerr << "--------------------\n";


}
//--------------------------------------------------------------------------------------------------



namespace excyrender { namespace Nature { namespace Et1 { namespace Backends { namespace {


    struct PythonPrinter final : AST::Visitor {

        PythonPrinter() {
            scope.push({""});
        }

        PythonPrinter(std::ostream &os) : os(os) {
            scope.push({""});
        }

        void begin(AST::Addition const &) { scope.push({" + "}); }
        void end(AST::Addition const &) { scope.pop(); }
        void begin(AST::Subtraction const &) { scope.push({" - "}); }
        void end(AST::Subtraction const &) { scope.pop(); }
        void begin(AST::Multiplication const &) { scope.push({" * "}); }
        void end(AST::Multiplication const &) { scope.pop(); }
        void begin(AST::Division const &) { scope.push({" / "}); }
        void end(AST::Division const &) { scope.pop(); }

        void begin(AST::LessThan const &)  { scope.push({" < "}); }
        void end(AST::LessThan const &)  { scope.pop(); }
        void begin(AST::LessEqual const &)  { scope.push({" <= "}); }
        void end(AST::LessEqual const &)  { scope.pop(); }
        void begin(AST::GreaterThan const &)  { scope.push({" > "}); }
        void end(AST::GreaterThan const &)  { scope.pop(); }
        void begin(AST::GreaterEqual const &)  { scope.push({" >= "}); }
        void end(AST::GreaterEqual const &)  { scope.pop(); }
        void begin(AST::Equal const &)  { scope.push({" == "}); }
        void end(AST::Equal const &)  { scope.pop(); }
        void begin(AST::NotEqual const &)  { scope.push({" != "}); }
        void end(AST::NotEqual const &)  { scope.pop(); }
        void begin(AST::LogicalAnd const &) { scope.push({" && "}); }
        void end(AST::LogicalAnd const &) { scope.pop(); }
        void begin(AST::LogicalOr const &) { scope.push({" || "}); }
        void end(AST::LogicalOr const &) { scope.pop(); }
        void begin(AST::LogicalNot const &) { os << " !"; }
        void end(AST::LogicalNot const &) {}

        void infix()
        {
            if (!scope.empty()) os << scope.top().Operator;
        }

        void begin(AST::Negation const &) { os << " -"; }
        void end(AST::Negation const &) { }

        void visit(AST::IntegerLiteral const &lit) { os << lit.value(); }
        void visit(AST::RealLiteral const &lit) { os << lit.value(); }
        void visit(AST::BoolLiteral const &lit) { os << lit.value(); }

        void visit(AST::Identifier const &id) { os << id.id(); }

        void begin(AST::Call const &call)
        {
            scope.push({", "});
            os << call.id() << "(";
        }
        void end(AST::Call const &)
        {
            os << ")";
            scope.pop();
        }

        void begin(AST::ParenExpression const &call) { os << "("; }
        void end(AST::ParenExpression const &) { os << ")"; }

        void begin(AST::Binding const &binding)
        {
            scope.push({""});
            if (!binding.arguments().empty()) {
                indent();
                os << "defun " << binding.id() << "(";

                bool first = true;
                for (auto a : binding.arguments()) {
                    if (!first) os << ", ";
                    first = false;
                    os << a.name;
                }
                os << "): return ";
            } else {
                indent();
                os << binding.id() << " = ";
            }
        }
        void end(AST::Binding const &)
        {
            scope.pop();
        }

        void begin(AST::IfThenElse const &ite)
        {
            os << "if ";
        }
        void before_then()
        {
            os << "\n";
            indent();
            os << "then ";
        }
        void before_else()
        {
            os << "\n";
            indent();
            os << "else ";
        }
        void end(AST::IfThenElse const &ite) {}

        void begin(AST::LetIn const &letin)
        {
            scope.push({"\n"});
        }
        void before_body(AST::LetIn const &)
        {
            os << "\n";
            indent();
            os << "return ";
        }
        void end(AST::LetIn const &)
        {
            scope.pop();
        }

        void begin(AST::Program const &id)
        {
            scope.push({"\n"});
            os << "def whew():\n";
            ++indent_;
        }
        void end(AST::Program const &)
        {
            scope.pop();
            --indent_;
        }

    private:
        std::ostream &os = std::cout;
        int indent_ = 0;
        void indent(int shift=0) {
            for (int i=0; i!=(indent_*2)+shift; ++i) {
                os << ' ';
            }
        }

        struct Operation {
            string Operator;
            int argCount = 0;

            Operation(string Operator) : Operator(Operator) {}
        };
        std::stack<Operation> scope;
    };


} } } } }


namespace excyrender { namespace Nature { namespace Et1 { namespace Backends {

string to_python(string et1)
{
    using namespace excyrender::Nature::Et1::detail;

    std::stringstream ss;
    PythonPrinter pp(ss);
    to_ast(et1)->accept(pp);
    return ss.str();
}

} } } }