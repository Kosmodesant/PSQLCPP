﻿#include <iostream>
#include <string>
#include <Wt/Dbo/Dbo.h>
#include <Wt/Dbo/backend/Postgres.h>
#include <Wt/WDateTime.h>

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

    Shops shops(Wt::Dbo::Transaction& t);
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

Shops Publisher::shops(Wt::Dbo::Transaction& t) {
    return t.session.query<Shop>("SELECT shop.* FROM shop INNER JOIN stock ON shop.id = stock.id_shop INNER JOIN book ON stock.id_book = book.id INNER JOIN publisher ON book.id_publisher = publisher.id WHERE publisher.name = ?").bind(name);
}

int main() {
    try {
        // Инициализация Wt::Dbo с использованием PostgreSQL
        Wt::Dbo::backend::Postgres postgresBackend("host=your_host user=your_user password=your_password dbname=your_dbname");
        Wt::Dbo::Session session;
        session.setConnection(postgresBackend);

        // Создание таблиц (если они не существуют)
        session.createTables();

        // Заполнение таблиц тестовыми данными (пример)
        Wt::Dbo::Transaction transaction(session);

        Wt::Dbo::ptr<Publisher> publisher1 = session.add(new Publisher{ "1", "Publisher1" });
        Wt::Dbo::ptr<Publisher> publisher2 = session.add(new Publisher{ "2", "Publisher2" });

        Wt::Dbo::ptr<Shop> shop1 = session.add(new Shop{ "1", "Shop1" });
        Wt::Dbo::ptr<Shop> shop2 = session.add(new Shop{ "2", "Shop2" });

        Wt::Dbo::ptr<Book> book1 = session.add(new Book{ "1", "Book1", publisher1 });
        Wt::Dbo::ptr<Book> book2 = session.add(new Book{ "2", "Book2", publisher2 });

        Wt::Dbo::ptr<Stock> stock1 = session.add(new Stock{ "1", book1, shop1, 10 });
        Wt::Dbo::ptr<Stock> stock2 = session.add(new Stock{ "2", book2, shop2, 5 });

        Wt::WDateTime now = Wt::WDateTime::currentDateTime();
        Wt::Dbo::ptr<Sale> sale1 = session.add(new Sale{ "1", 20.0, now, stock1, 2 });
        Wt::Dbo::ptr<Sale> sale2 = session.add(new Sale{ "2", 15.0, now, stock2, 3 });

        transaction.commit();

        // Ввод имени или идентификатора издателя
        std::string publisherName;
        std::cout << "Введите имя или идентификатор издателя: ";
        std::cin >> publisherName;

        // Выполнение запроса на выборку магазинов
        Wt::Dbo::Transaction queryTransaction(session);
        Wt::Dbo::ptr<Publisher> publisher = session.query<Publisher>().where("name = ?").bind(publisherName);
        Shops shops = publisher->shops(queryTransaction);


        // Вывод результатов
        std::cout << "Магазины, в которых продают книги издателя '" << publisherName << "':" << std::endl;
        for (const auto& shop : shops) {
            std::cout << "Магазин ID: " << shop.id << ", Название: " << shop.name << std::endl;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

