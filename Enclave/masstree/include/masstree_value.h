#pragma once

class Value {
    public:
        Value(int body_) : body(body_){};

        bool operator==(const Value &right) const {
            return body == right.body;
        }

        bool operator!=(const Value &right) const {
            return !operator==(right);
        }

        int getBody() const {
            return body;
        }

    private:
        int body;
};