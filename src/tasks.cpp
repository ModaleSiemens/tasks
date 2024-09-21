#include <iostream>
#include <print>
#include <thread>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

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

                bool addTask   (const std::vector<std::string> hierarchy, const std::string_view task);
                bool removeTask(const std::vector<std::string> hierarchy, const std::string_view task);
        
                void calculateTasksTotalTime(
                    const std::chrono::system_clock::time_point start,
                    const std::chrono::system_clock::time_point end
                );

                void renameTask(
                    const std::vector<std::string> hierarchy,
                    const std::string_view old_name,
                    const std::string_view new_name
                );

                void printTasksToTreeView(std::shared_ptr<app::Window> window);

                ~TasksManager();

            private:
                std::filesystem::path tasks_file_path;

                boost::property_tree::ptree tasks_tree;

                std::shared_ptr<app::Window> window;

                void parseTasksTree(
                    const boost::property_tree::ptree& property_tree,
                    const std::string_view key
                );
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

    try
    {
        tasks_manager.printTasksToTreeView(main_window);
    }
    catch(const boost::property_tree::json_parser::json_parser_error& e)
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
                    }
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

void TasksApp::TasksManager::printTasksToTreeView(std::shared_ptr<app::Window> window)
{
    std::ifstream tasks_file {tasks_file_path};

    boost::property_tree::read_json(tasks_file, tasks_tree);

    tasks_file.close();

    parseTasksTree(tasks_tree, "");
}

TasksApp::TasksManager::~TasksManager()
{
    std::ofstream tasks_file {tasks_file_path, std::ios::out | std::ios::trunc};

    boost::property_tree::write_json(tasks_file, tasks_tree);
}

void TasksApp::TasksManager::parseTasksTree(const boost::property_tree::ptree &property_tree, const std::string_view key)
{
    std::string next_key;

    if(!key.empty())
    {
        next_key = std::string{key} + '.';
    }

    std::vector<tgui::String> hierarchy;
    std::string temp_string;
    std::stringstream string_stream {std::string{key}};

    while(std::getline(string_stream, temp_string, '.'))
    {
        hierarchy.push_back(temp_string);
    }

    for(const auto& [current_key, data] : property_tree)
    {   
        hierarchy.push_back(current_key);

        window->getWidget<tgui::TreeView>("tasks_treeview")->addItem(
            hierarchy
        );

        hierarchy.pop_back();

        parseTasksTree(data, next_key + current_key);
    }
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
