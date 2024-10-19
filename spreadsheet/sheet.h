#pragma once

#include "cell.h"
#include "common.h"

#include <functional>
#include <vector>
#include <unordered_map>
#include <unordered_set>

struct PositionHasher {
    std::size_t operator()(const Position& pos) const noexcept {
        std::size_t hash_row = std::hash<int>()(pos.row);
        std::size_t hash_col = std::hash<int>()(pos.col);
        return hash_row ^ (hash_col << 1);
    }
};

class Cell;

class Sheet : public SheetInterface {
public:
    using SheetType = std::unordered_map<Position, std::unique_ptr<Cell>, PositionHasher>;

    ~Sheet();

    void SetCell(Position pos, std::string text) override;

    const CellInterface* GetCell(Position pos) const override;
    CellInterface* GetCell(Position pos) override;

    void ClearCell(Position pos) override;

    Size GetPrintableSize() const override;

    void PrintValues(std::ostream& output) const override;
    void PrintTexts(std::ostream& output) const override;

    void AddCellSource(Position pos, Cell* cell);
    void DeleteCellSource(Position pos, Cell* cell);

    bool IsCyclicRecursive(
        Position pos,
        std::unordered_set<Position, PositionHasher>& visited,
        std::unordered_set<Position, PositionHasher>& recursionStack,
        std::variant<std::nullptr_t, std::vector<Position>> start_edges
    ) const;

private:
    Size printable_size_;
    SheetType cells_;
};