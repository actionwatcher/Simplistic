#pragma once
enum error_t {
    kOK,
    kNoOp,
    kInvalidArgument,
    kUnknownCommand,
    kInvalidCommandArgument,
    kNotEnoughData, // not enough data in buffer
    kUnImplemented
};

enum wkstate_t {
    kSendingDIT,
    kSendingDAH,
    kWaiting,
    kDone
};