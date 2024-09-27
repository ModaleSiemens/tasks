#include <iostream>
#include <print>
#include <thread>
#include <boost/json/src.hpp>
#include <fstream>

#include "app.hpp"

class TasksApp : public app::Application 
{
    public:
        TasksApp();

        virtual void update(const app::Seconds delta_time) override;

        virtual ~TasksApp();

    private:
        class TasksManager
        {
            public:
                TasksManager(const std::filesystem::path file);

                void setWindow(std::shared_ptr<app::Window> window);

                bool addTask   (const std::vector<tgui::String> hierarchy, const std::string_view task);
                bool removeTask(const std::vector<tgui::String> hierarchy, const std::string_view task);
        
                void calculateTasksTotalTime(
                    const std::chrono::system_clock::time_point start,
                    const std::chrono::system_clock::time_point end
                );

                void renameTask(
                    const std::vector<tgui::String> hierarchy,
                    const std::string_view old_name,
                    const std::string_view new_name
                );

                void printTasksToTreeView(std::shared_ptr<app::Window> window);

                void loadFromFile();
                void saveToFile();

                ~TasksManager();

            private:
                boost::json::value& getTasksJsonValue(
                    const std::vector<tgui::String> hierarchy
                );

                void printTasksJson(
                    const boost::json::value& value,
                    const std::string_view path = ""
                );

                std::vector<tgui::String> getHierarchyFromString(const std::string_view str);

                std::filesystem::path tasks_file_path;

                boost::json::value tasks_json;

                std::shared_ptr<app::Window> window;
        };

        class CalendarManager
        {
            public:
                enum class TimeUnit
                {
                    days,
                    weeks,
                    months,
                    years
                };

                CalendarManager(const std::filesystem::path file);

                void setTimeUnit(const TimeUnit unit);

                void update(std::shared_ptr<app::Window> window);

                ~CalendarManager();
        };

        // Main window shortcut
        std::shared_ptr<app::Window> main_window;

        std::filesystem::path interfaces_path {"../assets/interfaces"};

        TasksManager    tasks_manager    {"../data/tasks.data"};
        //CalendarManager calendar_manager {"../data/calendar.data"};

        std::string getInterfacePath(const std::string_view interface_name);

        void setupMainInterface();

};

class Popup : public app::Window
{
    public:
        Popup(app::Application& app, const std::string_view interface_path);

        virtual ~Popup() = default;
};

int main()
{
    using namespace std::chrono_literals;
    using namespace std::chrono;

    TasksApp app;

    steady_clock::time_point new_timepoint {steady_clock::now()};
    steady_clock::time_point old_timepoint;

    while(app.getWindow("main"))
    {
        old_timepoint = new_timepoint;
        new_timepoint = steady_clock::now();

        app.update(new_timepoint - old_timepoint);

        std::this_thread::sleep_for(1.0s / 60.0);

    }

    return 0;
}

TasksApp::TasksApp()
{
    addWindow(
        "main",
        true,
        getInterfacePath("main"),
        sf::VideoMode(1200, 800),
        "Tasks (studying time managing program)",
        sf::Style::Close
    );
    
    main_window = getWindow("main");

    tasks_manager.setWindow(main_window);

    try
    {
        tasks_manager.loadFromFile();
    }
    catch(const boost::json::system_error& e)
    {
        addWindow<Popup>("popup", true, getInterfacePath("error"));

        auto popup {getWindow("popup")};

        popup->setTitle("Failed to load tasks!");
        popup->getWidget<tgui::TextArea>("message_textarea")->setText(
            std::format("Failed to parse tasks JSON data!\nException: \"{}\".", e.what())
        );
        popup->getWidget<tgui::Button>("ok_button")->setText("Quit");
        popup->getWidget<tgui::Button>("ok_button")->onClick(
            []
            {
                std::exit(1);
            }
        );
    }


    setupMainInterface();
}

void TasksApp::update(const app::Seconds delta_time)
{
    //tasks_manager.update(*main_window, delta_time);
    //calendar_manager.update(*main_window, delta_time);
    
    Application::update(delta_time);
}

TasksApp::~TasksApp()
{

}

std::string TasksApp::getInterfacePath(const std::string_view interface_name)
{
    return (interfaces_path / (std::string{interface_name} + ".txt")).string();
}

void TasksApp::setupMainInterface()
{
    auto tasks_treeview {main_window->getWidget<tgui::TreeView>("tasks_treeview")};

    tasks_manager.printTasksToTreeView(main_window);

    tasks_treeview->onRightClick(
        [=, this]
        {
            if( 
                const auto menu_listbox {main_window->getWidget("menu_listbox")}
            )
            {
                main_window->remove(menu_listbox);         
            }

            sf::Mouse mouse;

            auto menu {tgui::ListBox::create()};

            menu->setPosition(mouse.getPosition(*main_window).x, mouse.getPosition(*main_window).y);

            menu->addItem("Start Timer", "timer");
            menu->addItem("Start Countdown", "countdown");
            menu->addItem("Rename Task", "rename");
            menu->addItem("Delete Task", "delete");
            menu->addItem("Exit Menu", "exit");

            menu->onDoubleClick(
                [=, this]
                {
                    const auto selected_item {menu->getSelectedItemId()};

                    if(selected_item == "exit")
                    {
                        main_window->remove(menu);

                        return;
                    }
                    else
                    {
                        auto task {tasks_treeview->getSelectedItem()};
                        const auto task_name {task.back()};
                        task.pop_back();

                        if(selected_item == "delete")
                        {
                            tasks_manager.removeTask(task, task_name.toStdString());
                        }

                        if(selected_item == "rename")
                        {
                        }
                        
                    }

                    main_window->remove(menu);
                }
            );

            main_window->add(menu, "menu_listbox");                
        }
    );
}

TasksApp::TasksManager::TasksManager(const std::filesystem::path file_path)
:
    tasks_file_path{file_path}
{
}

void TasksApp::TasksManager::setWindow(std::shared_ptr<app::Window> t_window)
{
    window = t_window;
}

bool TasksApp::TasksManager::removeTask(const std::vector<tgui::String> hierarchy, const std::string_view task)
{

}

void TasksApp::TasksManager::printTasksToTreeView(std::shared_ptr<app::Window> window)
{
    printTasksJson(tasks_json);
}

void TasksApp::TasksManager::loadFromFile()
{
    std::ifstream file (tasks_file_path);

    std::stringstream buffer;

    buffer << file.rdbuf();

    tasks_json = boost::json::parse(buffer.str());
}

void TasksApp::TasksManager::saveToFile()
{
    std::ofstream file (tasks_file_path);

    file << boost::json::serialize(tasks_json);
}

TasksApp::TasksManager::~TasksManager()
{
    saveToFile();
}

void TasksApp::TasksManager::printTasksJson(const boost::json::value &value, const std::string_view path)
{
    std::println("{}", path);

    if(value.is_object())
    {
        const boost::json::object& object {value.as_object()};

        for(const auto&[key, value] : object)
        {
            auto hierarchy {getHierarchyFromString(path)};

            hierarchy.push_back(tgui::String{key});

            window->getWidget<tgui::TreeView>("tasks_treeview")->addItem(
                hierarchy
            );

            printTasksJson(value, std::string{path} + (path == "" ? "" : ".") + std::string{key});
        }
    }
}

std::vector<tgui::String> TasksApp::TasksManager::getHierarchyFromString(const std::string_view str)
{
    std::vector<tgui::String> hierarchy;

    std::string temp;
    std::stringstream stream {std::string{str}};

    while(std::getline(stream, temp, '.'))
    {
        hierarchy.push_back(temp);
    }

    return hierarchy;
}

Popup::Popup(app::Application &app, const std::string_view interface_path)
:
    Window{
        app,
        interface_path,
        sf::VideoMode(500, 250),
        "Alert!",
        sf::Style::Titlebar
    }
{
}
