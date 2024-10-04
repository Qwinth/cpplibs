#pragma once

class FileDescriptor {
protected:
    int desc;

public:
    FileDescriptor() = default;
    FileDescriptor(int desc) : desc(desc) {};

    virtual ~FileDescriptor() = default;
    virtual int fd() const { return desc; }
    
};