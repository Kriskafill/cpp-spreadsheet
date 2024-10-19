#include "cell.h"

#include <cassert>
#include <iostream>
#include <string>
#include <optional>

class Cell::Impl {
public:
    using Value = std::variant<std::string, double, FormulaError>;

    virtual ~Impl() = default;
    virtual Value GetValue() const = 0;
    virtual std::string GetText() const = 0;
};

class Cell::EmptyImpl : public Cell::Impl {
public:
    explicit EmptyImpl() = default;

    Value GetValue() const override {
        return "";
    }

    std::string GetText() const override {
        return "";
    }
};

class Cell::TextImpl : public Cell::Impl {
public:
    explicit TextImpl(std::string text) : text_(std::move(text)) {}

    Value GetValue() const override {
        if (!text_.empty() && text_.front() == '\'') {
            return text_.substr(1);
        }
        return text_;
    }

    std::string GetText() const override {
        return text_;
    }

private:
    std::string text_;
};

class Cell::FormulaImpl : public Cell::Impl {
public:
    explicit FormulaImpl(std::string text, const SheetInterface& sheet)
        : formula_(ParseFormula(std::move(text)))
        , sheet_(sheet) { }

    Value GetValue() const override {
        if (cash_.has_value()) {
            return cash_.value();
        }

        auto evaluate = formula_->Evaluate(sheet_);
        if (std::get_if<double>(&evaluate)) {
            cash_ = std::get<double>(evaluate);
            return std::get<double>(evaluate);
        }
        else {
            cash_ = std::get<FormulaError>(evaluate);
            return std::get<FormulaError>(evaluate);
        }
    }

    std::string GetText() const override {
        return '=' + formula_->GetExpression();
    }

    const FormulaInterface* GetFormula() const {
        return formula_.get();
    }

    void InvalidateCash() const {
        cash_.reset();
    }

private:
    std::unique_ptr<FormulaInterface> formula_;
    const SheetInterface& sheet_;
    mutable std::optional<Value> cash_;
};

Cell::Cell(Sheet& sheet, Position& pos)
    : impl_(std::make_unique<EmptyImpl>())
    , sheet_(sheet)
    , position_(pos) { }

Cell::~Cell() {
    impl_ = nullptr;
}

Cell::Value Cell::GetValue() const {
    return impl_->GetValue();
}

std::string Cell::GetText() const {
    return impl_->GetText();
}

std::vector<Position> Cell::GetReferencedCells() const {
    if (FormulaImpl* formula_ptr = dynamic_cast<FormulaImpl*>(impl_.get())) {
        return formula_ptr->GetFormula()->GetReferencedCells();
    }
    else {
        return {};
    }
}

void Cell::Set(std::string text) {

    if (text.empty()) {
        for (const Position& reference_cell : GetReferencedCells()) {
            sheet_.DeleteCellSource(reference_cell, this);
        }
        impl_ = std::make_unique<EmptyImpl>();
    }
    else if (text.front() == FORMULA_SIGN && text.size() > 1) {
        auto temp_formula = ParseFormula(text.substr(1));
        IsCyclic(temp_formula->GetReferencedCells());

        for (const Position& reference_cell : GetReferencedCells()) {
            sheet_.DeleteCellSource(reference_cell, this);
        }
        impl_ = std::make_unique<FormulaImpl>(std::move(text.substr(1)), sheet_);
    }
    else {
        for (const Position& reference_cell : GetReferencedCells()) {
            sheet_.DeleteCellSource(reference_cell, this);
        }
        impl_ = std::make_unique<TextImpl>(std::move(text));
    }

    for (const Position& reference_cell : GetReferencedCells()) {
        sheet_.AddCellSource(reference_cell, this);
    }

    for (const Cell* source : sources_) {
        source->InvalidateCash();
    }
}

void Cell::Clear() {
    impl_ = std::make_unique<EmptyImpl>();
}

void Cell::AddSource(Cell* cell) {
    sources_.insert(cell);
}

void Cell::DeleteSource(Cell* cell) {
    sources_.erase(cell);
}

size_t Cell::GetCountSources() const {
    return sources_.size();
}

bool Cell::IsEmpty() const {
    return dynamic_cast<EmptyImpl*>(impl_.get()) != nullptr;
}

void Cell::IsCyclic(std::vector<Position> start_edges) const {
    std::unordered_set<Position, PositionHasher> visited;
    std::unordered_set<Position, PositionHasher> recursionStack;

    if (sheet_.IsCyclicRecursive(position_, visited, recursionStack, std::move(start_edges))) {
        throw CircularDependencyException("Cyclic");
    }
}

void Cell::InvalidateCash() const {
    if (impl_ && dynamic_cast<FormulaImpl*>(impl_.get()) != nullptr) {
        dynamic_cast<FormulaImpl*>(impl_.get())->InvalidateCash();
    }

    for (const Cell* source : sources_) {
        source->InvalidateCash();
    }
}