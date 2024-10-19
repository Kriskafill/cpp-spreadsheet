#include "formula.h"

#include "FormulaAST.h"

#include <algorithm>
#include <cassert>
#include <cctype>
#include <sstream>

using namespace std::literals;

std::ostream& operator<<(std::ostream& output, FormulaError fe) {
    return output << fe.ToString();
}

namespace {
    class Formula : public FormulaInterface {
    public:

        explicit Formula(std::string expression) try
            : ast_(ParseFormulaAST(expression)) {
        }
        catch (const std::exception& exc) {
            std::throw_with_nested(FormulaException(exc.what()));
        }

        Value Evaluate(const SheetInterface& sheet) const override {
            try {
                auto get_cell_value = [&sheet](const Position* pos) -> CellInterface::Value {
                    if (sheet.GetCell(*pos)) {
                        return sheet.GetCell(*pos)->GetValue();
                    }
                    else {
                        return 0.0;
                    }
                    };
                return ast_.Execute(get_cell_value);
            }
            catch (FormulaError& err) {
                return err;
            }
        }

        std::string GetExpression() const override {
            std::ostringstream out;
            ast_.PrintFormula(out);
            return out.str();
        }

        std::vector<Position> GetReferencedCells() const {
            std::vector<Position> vect;
            std::copy(ast_.GetCells().begin(), ast_.GetCells().end(), std::back_inserter(vect));

            auto last = std::unique(vect.begin(), vect.end());
            vect.erase(last, vect.end());

            return vect;
        }

    private:
        FormulaAST ast_;
    };
}

std::unique_ptr<FormulaInterface> ParseFormula(std::string expression) {
    return std::make_unique<Formula>(std::move(expression));
}