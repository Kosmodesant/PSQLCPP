#include <string>
#include <Wt/Dbo/Dbo>
#include <Wt/Dbo/WtSqlTraits>

class Book {
public:
    int id;
    std::string title;
    Wt::Dbo::ptr<Publisher> publisher;

    template<class Action>
    void persist(Action& a) {
        Wt::Dbo::field(a, id, "id");
        Wt::Dbo::field(a, title, "title");
        Wt::Dbo::belongsTo(a, publisher, "id_publisher");
    }
};