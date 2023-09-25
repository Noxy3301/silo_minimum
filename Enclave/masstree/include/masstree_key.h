#pragma once

#include <cstdint>
#include <vector>
#include <cassert>

// CHECK: KeyWithSliceって何に使うんだ？

struct SliceWithSize {
    uint64_t slice; // スライスの実際のデータ
    uint8_t size;   // スライスのサイズ

    SliceWithSize(uint64_t slice_, uint8_t size_) : slice(slice_), size(size_) {
        assert(1 <= size && size <= 8);
    }
    // 2つのSliceWithSizeオブジェクトが等しいかどうかを確認する
    bool operator==(const SliceWithSize &right) const {
        return slice == right.slice && size == right.size;
    }
    // 2つのSliceWithSizeオブジェクトが異なるかどうかを確認する
    bool operator!=(const SliceWithSize &right) const {
        return !(*this == right);
    }
};

class Key {
    public:
        std::vector<uint64_t> slices;   // スライスのリスト
        size_t lastSliceSize = 0;       // 最後のスライスのサイズ
        size_t cursor = 0;              // 現在のスライスの位置/インデックス

        Key(std::vector<uint64_t> slices_, size_t lastSliceSize_) : slices(std::move(slices_)), lastSliceSize(lastSliceSize_) {
            assert(1 <= lastSliceSize && lastSliceSize <= 8);
        }
        // 次のスライスが存在するかどうかを確認する
        bool hasNext() const {
            if (slices.size() == cursor + 1) return false;
            return true;
        }
        // 指定された位置からの残りのスライスの長さを取得する
        size_t remainLength(size_t from) const {
            assert(from <= slices.size() - 1);
            return (slices.size() - from - 1)*8 + lastSliceSize;
        }
        // 現在のスライスのサイズを取得する
        size_t getCurrentSliceSize() const {
            if (hasNext()) {
                return 8;
            } else {
                return lastSliceSize;
            }
        }
        // 現在のスライスとカーソル(スライスの位置/インデックス)を取得する
        SliceWithSize getCurrentSlice() const {
            return SliceWithSize(slices[cursor], getCurrentSliceSize());
        }
        // 次のスライスに進む
        void next() {
            assert(hasNext());
            cursor++;
        }
        // 前のスライスに戻る
        void back() {
            assert(cursor != 0);
            cursor--;
        }
        // カーソルをリセットする
        void reset() {
            cursor = 0;
        }
        // 2つのKeyオブジェクトが等しいかどうかを確認する
        bool operator==(const Key &right) const {
            return lastSliceSize == right.lastSliceSize && cursor == right.cursor && slices == right.slices;
        }
        // 2つのKeyオブジェクトが異なるかどうかを確認する
        bool operator!=(const Key &right) const {
            return !(*this == right);
        }
};