uint32_t hexToUint(const string& h)
{
    return static_cast<uint32_t>(stoul(h, nullptr, 16));
}
int32_t signExtend(uint32_t value, int bits)
{
    int32_t m = 1u << (bits - 1);
    return (value ^ m) - m;
}
string to_hex(uint32_t x, int width=8)
{
    stringstream ss; ss << setfill('0') << setw(width) << hex << nouppercase << x;
    return ss.str();
}

