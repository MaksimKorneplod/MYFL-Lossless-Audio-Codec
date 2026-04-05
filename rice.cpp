#include "rice.hpp"

std::vector<uint8_t> riceEncode(const std::vector<int>& residuals, int param) {
    std::vector<uint8_t> encoded;
    encoded.reserve(residuals.size() * 2);
    uint8_t current_byte = 0;
    int bit_pos = 7;

    auto push_bit = [&](bool bit) {
        if (bit) current_byte |= (1 << bit_pos);
        if (--bit_pos < 0) {
            encoded.push_back(current_byte);
            current_byte = 0;
            bit_pos = 7;
        }
    };

    for (int val : residuals) {
        // ZigZag-кодирование знака
        unsigned int zz_val = zigZagEncode(val);

        // Унарная часть: q нулей + единица
        int q = zz_val >> param;
        for (int i = 0; i < q; ++i) {
            push_bit(false);
        }
        push_bit(true);

        // Бинарная часть: r в k битах
        int r = zz_val & ((1 << param) - 1);
        for (int i = param - 1; i >= 0; --i) {
            push_bit((r >> i) & 1);
        }
    }

    if (bit_pos != 7) {
        encoded.push_back(current_byte);
    }

    return encoded;
}

std::vector<int> riceDecode(const std::vector<uint8_t>& encoded, int param) {
    std::vector<int> residuals;
    if (encoded.empty() || param < 0 || param > 16)
        return residuals;

    size_t byteIdx = 0;
    int bitPos = 7;
    uint8_t currentByte = encoded[byteIdx];

    auto nextBit = [&]() -> int {
        if (byteIdx >= encoded.size()) return -1;
        int bit = (currentByte >> bitPos) & 1;
        bitPos--;
        if (bitPos < 0) {
            byteIdx++;
            if (byteIdx < encoded.size()) {
                currentByte = encoded[byteIdx];
                bitPos = 7;
            }
        }
        return bit;
    };

    while (byteIdx < encoded.size()) {
        // Унарная часть: считаем q
        int q = 0;
        while (true) {
            int bit = nextBit();
            if (bit == -1) return residuals;
            if (bit) break;
            q++;
        }

        // Бинарная часть: читаем r
        int r = 0;
        for (int i = 0; i < param; ++i) {
            int bit = nextBit();
            if (bit == -1) return residuals;
            r = (r << 1) | bit;
        }

        // ZigZag-декодирование
        unsigned int zz_val = (q << param) | r;
        residuals.push_back(zigZagDecode(zz_val));
    }

    return residuals;
}

size_t estimateRiceSize(const std::vector<int32_t>& residuals, int param) {
    size_t totalBits = 0;
    for (int32_t val : residuals) {
        uint32_t absVal = zigZagEncode(val);
        totalBits += (absVal >> param) + 1 + param;
    }
    return (totalBits + 7) / 8;
}
