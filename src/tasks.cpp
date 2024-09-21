#include <iostream>
#include "app.hpp"

class TasksApp : public app::Application 
{
    public:

};

int main()
{
    using namespace std::chrono_literals;
    using namespace std::chrono;

    TasksApp app;

    steady_clock::time_point new_timepoint {steady_clock::now()};
    steady_clock::time_point old_timepoint;

    while(true)
    {
        old_timepoint = new_timepoint;
        new_timepoint = steady_clock::now();

        app.update(new_timepoint - old_timepoint);
    }

    return 0;
} 