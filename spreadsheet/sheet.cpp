#include "sheet.h"

#include "cell.h"
#include "common.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <optional>

using namespace std::literals;

Sheet::~Sheet() { }

void Sheet::SetCell(Position pos, std::string text) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("");
    }

    if (cells_.count(pos) > 0) {
        cells_.at(pos)->Set(std::move(text));
    }
    else {
        cells_.insert({ pos, std::make_unique<Cell>(*this, pos) });
        cells_.at(pos)->Set(std::move(text));
    }

    printable_size_.rows = std::max(printable_size_.rows, pos.row + 1);
    printable_size_.cols = std::max(printable_size_.cols, pos.col + 1);
}

const CellInterface* Sheet::GetCell(Position pos) const {
    if (!pos.IsValid()) {
        throw InvalidPositionException("");
    }

    if (cells_.count(pos) > 0) {
        return cells_.at(pos).get();
    }
    else {
        return nullptr;
    }
}

CellInterface* Sheet::GetCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("");
    }

    if (cells_.count(pos) > 0) {
        return cells_.at(pos).get();
    }
    else {
        return nullptr;
    }
}

void Sheet::ClearCell(Position pos) {
    if (!pos.IsValid()) {
        throw InvalidPositionException("");
    }

    if (cells_.count(pos) > 0) {
        cells_.at(pos)->Clear();
    }
    DeleteCellSource(pos, nullptr);

    printable_size_ = { 0,0 };
    for (auto& cell : cells_) {
        if (!cell.second->IsEmpty()) {
            printable_size_.rows = std::max(printable_size_.rows, cell.first.row + 1);
            printable_size_.cols = std::max(printable_size_.cols, cell.first.col + 1);
        }
    }
}

Size Sheet::GetPrintableSize() const {
    return printable_size_;
}

void Sheet::PrintValues(std::ostream& output) const {

    for (int i = 0; i < printable_size_.rows; ++i) {
        for (int j = 0; j < printable_size_.cols; ++j) {
            if (cells_.count({ i, j }) > 0) {
                Cell::Value value = cells_.at({ i, j })->GetValue();
                if (std::holds_alternative<std::string>(value)) {
                    output << std::get<std::string>(value);
                }
                else if (std::holds_alternative<double>(value)) {
                    output << std::get<double>(value);
                }
                else {
                    output << std::get<FormulaError>(value);
                }
            }

            if (j < printable_size_.cols - 1) {
                output << '\t';
            }
        }
        output << '\n';
    }
}

void Sheet::PrintTexts(std::ostream& output) const {
    for (int i = 0; i < printable_size_.rows; ++i) {
        for (int j = 0; j < printable_size_.cols; ++j) {
            if (cells_.count({ i, j }) > 0) {
                output << cells_.at({ i, j })->GetText();
            }

            if (j < printable_size_.cols - 1) {
                output << '\t';
            }
        }
        output << '\n';
    }
}

void Sheet::AddCellSource(Position pos, Cell* cell) {
    if (cells_.count(pos) == 0) {
        cells_.insert({ pos, std::make_unique<Cell>(*this, pos) });
        cells_.at(pos)->Set("");
    }
    cells_.at(pos)->AddSource(cell);
}

void Sheet::DeleteCellSource(Position pos, Cell* cell) {
    if (cells_.count(pos) > 0) {
        cells_.at(pos)->DeleteSource(cell);

        if (cells_.at(pos)->IsEmpty() && cells_.at(pos)->GetCountSources() == 0) {
            cells_.erase(pos);
        }
    }
}

bool Sheet::IsCyclicRecursive(
    Position pos,
    std::unordered_set<Position, PositionHasher>& visited,
    std::unordered_set<Position, PositionHasher>& recursionStack,
    std::variant<std::nullptr_t, std::vector<Position>> start_edges
) const {

    if (recursionStack.count(pos) > 0) {
        return true;
    }

    if (visited.count(pos) > 0) {
        return false;
    }

    recursionStack.insert(pos);
    visited.insert(pos);

    if (std::holds_alternative<std::nullptr_t>(start_edges)) {
        Cell* cell = cells_.at(pos).get();
        std::vector<Position> referencedCells = cell->GetReferencedCells();
        for (const Position& refPos : referencedCells) {
            if (cells_.count(refPos) == 0) {
                continue;
            }
            if (IsCyclicRecursive(refPos, visited, recursionStack, nullptr)) {
                return true;
            }
        }
    }
    else {
        for (const Position& refPos : std::get<std::vector<Position>>(start_edges)) {
            if (cells_.count(refPos) == 0) {
                continue;
            }
            if (IsCyclicRecursive(refPos, visited, recursionStack, nullptr)) {
                return true;
            }
        }
    }

    recursionStack.erase(pos);
    return false;
}

std::unique_ptr<SheetInterface> CreateSheet() {
    return std::make_unique<Sheet>();
}