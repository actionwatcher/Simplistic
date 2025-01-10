class FIFOBuffer {
private:
    char* buffer;
    int capacity;
    int front;
    int rear;
    int count;

public:
    FIFOBuffer(int size = 128);
    ~FIFOBuffer();
    void append(char c);
    void putback(); //Undo last get. Unsafe unless atomic relative to get(s).
    void drop();
    char get();
    char peek() const;
    int size() const;
    bool is_full() const;
    void clear();
};
