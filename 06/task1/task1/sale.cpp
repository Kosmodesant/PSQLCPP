#include <string>
#include <Wt/Dbo/Dbo>
#include <Wt/Dbo/WtSqlTraits>

class Sale {
public:
    int id;
    double price;
    Wt::Dbo::DateTime date_sale;
    Wt::Dbo::ptr<Stock> stock;
    int count;

    template<class Action>
    void persist(Action& a) {
        Wt::Dbo::field(a, id, "id");
        Wt::Dbo::field(a, price, "price");
        Wt::Dbo::field(a, date_sale, "date_sale");
        Wt::Dbo::belongsTo(a, stock, "id_stock");
        Wt::Dbo::field(a, count, "count");
    }
};