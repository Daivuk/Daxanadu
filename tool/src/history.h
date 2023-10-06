#pragma once

#include <vector>


template<typename T>
class History final
{
public:
    void push_undo(const T& state)
    {
        if (m_history_point < (int)m_history.size() - 1)
            m_history.erase(m_history.begin() + (m_history_point + 1), m_history.end());
        m_history.push_back(state);
        m_history_point = (int)m_history.size() - 1;
    }

    void undo(T& state)
    {
        if (m_history_point > 0)
        {
            m_history_point--;
            state = m_history[m_history_point];
        }
    }

    void redo(T& state)
    {
        if (m_history_point < (int)m_history.size() - 1)
        {
            m_history_point++;
            state = m_history[m_history_point];
        }
    }

private:
    std::vector<state_t> m_history;
    int m_history_point = 0;
};
