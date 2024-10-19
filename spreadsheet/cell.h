#pragma once

#include "common.h"
#include "formula.h"
#include "sheet.h"

#include <functional>
#include <unordered_set>

class Sheet;

class Cell : public CellInterface {
public:
    Cell(Sheet& sheet, Position& pos);
    ~Cell();

    Value GetValue() const override;
    std::string GetText() const override;
    std::vector<Position> GetReferencedCells() const override;

    void Set(std::string text);
    void Clear();

    void AddSource(Cell* cell);
    void DeleteSource(Cell* cell);
    size_t GetCountSources() const;

    bool IsEmpty() const;
    void IsCyclic(std::vector<Position> start_edges) const;

    void InvalidateCash() const;

private:
    class Impl;
    class EmptyImpl;
    class TextImpl;
    class FormulaImpl;

    std::unique_ptr<Impl> impl_;
    std::unordered_set<Cell*> sources_;

    Sheet& sheet_;
    Position position_;

};