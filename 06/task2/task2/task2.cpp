#include <iostream>
#include <string>
#include <Wt/Dbo/Dbo.h>
#include <Wt/Dbo/backend/Postgres.h>
#include <Wt/WDateTime.h>
#include <Wt/Dbo/Query.h>
#include <Wt/Dbo/QueryModel.h>

// Определение классов Publisher, Book, Stock, Sale, Shop
class Publisher;
class Book;
class Stock;
class Sale;
class Shop;

typedef Wt::Dbo::collection<Wt::Dbo::ptr<Shop>> Shops;

class Publisher {
public:
    int id;
    std::string name;

    template<class Action>
    void persist(Action& a) {
        Wt::Dbo::field(a, id, "id");
        Wt::Dbo::field(a, name, "name");
    }

    Shops shops(Wt::Dbo::Transaction& t) const;
};

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

class Stock {
public:
    int id;
    Wt::Dbo::ptr<Book> book;
    Wt::Dbo::ptr<Shop> shop;
    int count;

    template<class Action>
    void persist(Action& a) {
        Wt::Dbo::field(a, id, "id");
        Wt::Dbo::belongsTo(a, book, "id_book");
        Wt::Dbo::belongsTo(a, shop, "id_shop");
        Wt::Dbo::field(a, count, "count");
    }
};

class Sale {
public:
    int id;
    double price;
    Wt::WDateTime date_sale;
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

class Shop {
public:
    int id;
    std::string name;

    template<class Action>
    void persist(Action& a) {
        Wt::Dbo::field(a, id, "id");
        Wt::Dbo::field(a, name, "name");
    }
};

Shops Publisher::shops(Wt::Dbo::Transaction& t) const {
    Wt::Dbo::Query<Wt::Dbo::ptr<Shop>> query = t.session().query<Wt::Dbo::ptr<Shop>>("shop.id");
    query.join("INNER JOIN stock ON shop.id = stock.id_shop");
    query.join("INNER JOIN book ON stock.id_book = book.id");
    query.join("INNER JOIN publisher ON book.id_publisher = publisher.id");
    query.where("publisher.name = ?").bind(name.c_str());
 
    return query.resultList();
}

int main() {
    try {
        // Инициализация Wt::Dbo с использованием PostgreSQL
        std::unique_ptr<Wt::Dbo::backend::Postgres> postgresBackend =
            std::make_unique<Wt::Dbo::backend::Postgres>("user=username password=PASSWORD dbname=dbname");
        Wt::Dbo::Session session;
        session.setConnection(std::move(postgresBackend));

        // Создание таблиц (если они не существуют)
        session.createTables();

        // Заполнение таблиц тестовыми данными (пример)
        Wt::Dbo::Transaction transaction(session);

        Wt::Dbo::ptr<Publisher> publisher1 = session.add(std::make_unique<Publisher>(Publisher{ 0, "Publisher1" }));
        Wt::Dbo::ptr<Publisher> publisher2 = session.add(std::make_unique<Publisher>(Publisher{ 0, "Publisher2" }));

        Wt::Dbo::ptr<Shop> shop1 = session.add(std::make_unique<Shop>(Shop{ 0, "Shop1" }));
        Wt::Dbo::ptr<Shop> shop2 = session.add(std::make_unique<Shop>(Shop{ 0, "Shop2" }));

        Wt::Dbo::ptr<Book> book1 = session.add(std::make_unique<Book>(Book{ 0, "Book1", publisher1 }));
        Wt::Dbo::ptr<Book> book2 = session.add(std::make_unique<Book>(Book{ 0, "Book2", publisher2 }));

        Wt::Dbo::ptr<Stock> stock1 = session.add(std::make_unique<Stock>(Stock{ 0, book1, shop1, 10 }));
        Wt::Dbo::ptr<Stock> stock2 = session.add(std::make_unique<Stock>(Stock{ 0, book2, shop2, 5 }));

        Wt::WDateTime now = Wt::WDateTime::currentDateTime();
        Wt::Dbo::ptr<Sale> sale1 = session.add(std::make_unique<Sale>(Sale{ 0, 20.0, now, stock1, 2 }));
        Wt::Dbo::ptr<Sale> sale2 = session.add(std::make_unique<Sale>(Sale{ 0, 15.0, now, stock2, 3 }));

        transaction.commit();

        // Ввод имени или идентификатора издателя
        std::string publisherName;
        std::cout << "Введите имя или идентификатор издателя: ";
        std::cin >> publisherName;

        // Выполнение запроса на выборку магазинов
        Wt::Dbo::Transaction queryTransaction(session);

        Wt::Dbo::ptr<Publisher> publisher = session.query<Wt::Dbo::ptr<Publisher>>(
            "SELECT * FROM publisher WHERE name = ?").bind(publisherName.c_str()).resultValue();

        Shops shops = publisher->shops(queryTransaction);

        // Вывод результатов
        std::cout << "Магазины, в которых продают книги издателя '" << publisherName << "':" << std::endl;
        for (const auto& shop : shops) {
            std::cout << "Магазин ID: " << shop->id << ", Название: " << shop->name << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
