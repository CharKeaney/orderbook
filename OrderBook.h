
#ifndef ORDERBOOK_H
#define ORDERBOOK_H

#include <cstdint>
#include <time.h>
#include <string>
#include <iostream>
using namespace std;

#define MAX_NUM_ORDERS_IN_HEAP (1 << 16)
#define NUM_TOTAL_SYMBOLS_IN_ORDER_BOOK (1 << 16)

typedef enum error_status {
	ACCEPT,
	INVALID_AMMENDMENT_DETAILS = 101,
	INVALID_ORDER_DETAILS = 303,
	ORDER_DOES_NOT_EXIST = 404,
};

typedef enum action {
	NEW,
	AMEND,
	CANCEL,
	MATCH,
	QUERY
} action_t;

typedef uint64_t order_id_t;

typedef string symbol_t;

typedef enum order_type {
	MARKET,
	LIMIT,
	IOC
} order_type_t;


static const char* order_type_to_str[]{
	"M",
	"L",
	"I"
};

typedef enum side {
	BUY,
	SELL
} side_t;

typedef float price_t;

typedef uint64_t quantity_t;

typedef enum execution_status_t {
	NOT_EXECUTED,
	PARTIALLY_EXECUTED,
	EXECUTED,
	CANCELLED
} execution_status_t;

typedef struct alteration_history_t {
	execution_status_t history;
	time_t date;	
	price_t price;
	quantity_t quantity_remaining;
	alteration_history_t* next_update;
} alteration_history_t;


/* Class representing an order, buy or sell, 
   which has an ID, a type, and history that 
   tracks change to itself. */
class Order {
private:
	order_id_t order_id;
	order_type_t order_type;
	alteration_history_t* history;
public:
	Order(
		order_id_t oid,
		order_type_t ot,
		time_t t,
		price_t p,
		quantity_t q) {

		order_id = oid;
		order_type = ot;
--		history = new alteration_history_t{
			NOT_EXECUTED,
			t,
			p,
			q,
			NULL
		};
	}

	Order()
		: Order(
			(order_id_t) 0,
			(order_type_t) 0,
			(time_t) 0,
			(price_t) 0,
			(quantity_t) 0) {
	};

	bool active_at(const time_t& t = -1) const {
		
		bool is_active = false;
		
		alteration_history_t* ht = get_history_entry(t);
		execution_status_t status = ht->history;
		if ((t == -1 || history->date <= t)
			&& status != EXECUTED  
			&& status != CANCELLED) {
			is_active = true;
		}
		
		return is_active;
	}

	alteration_history_t *get_history_entry(const time_t& t = -1) const {
		alteration_history_t* most_recent = history; 
		for (alteration_history_t* eht = history;
			eht != NULL;
			eht = eht->next_update) {
			if (t == -1 || eht->date <= t) {
				most_recent = eht; 
			} else {
				break;
			}
		}
		return most_recent;
	}

	execution_status_t get_status(const time_t& t = -1) const{
		return get_history_entry(t)->history;
	}

	price_t get_price(const time_t& t = -1) const {
		return get_history_entry(t)->price;
	}

	quantity_t get_quantity(const time_t& t = -1) const {
		return get_history_entry(t)->quantity_remaining;
	}

	time_t get_timestamp(const time_t& t = -1) const{
		return get_history_entry(t)->date;
	}

	order_id_t get_order_id() const {
		return order_id;
	}

	order_type_t get_order_type() const {
		return order_type;
	}

	int add_execution_history_entry(
		const execution_status_t& history,
		const time_t& date,
		const price_t& price,
		const quantity_t& quantity_remaining) {

		alteration_history_t* new_entry = new alteration_history_t {
			history,
			date,
			price,
			quantity_remaining,
			NULL
		};

		alteration_history_t* history_entry = get_history_entry();
		if (history_entry->date > date) {
			delete new_entry;
		} else {
			history_entry->next_update = new_entry;
		}
		return ACCEPT;
	}

	Order transact(
		const time_t& date, 
		const price_t& new_price, 
		const quantity_t& new_quantity) {

		execution_status_t status = (new_quantity == 0)
			? EXECUTED : PARTIALLY_EXECUTED;
		
		add_execution_history_entry(
			status,
			date, 
			new_price, 
			new_quantity);
		
		return *this;
	}

	int ammend(
		const price_t& new_price, 
		const quantity_t& new_quantity) {

		alteration_history_t* history_entry = get_history_entry();
		
		add_execution_history_entry(
			history_entry->history,
			history_entry->date,
			new_price,
			new_quantity);
		return ACCEPT;
	}
};

typedef enum order_heap_type_t {
	BUY_HEAP,
	SELL_HEAP
} order_heap_type_t;


/* Min-Max heap used to represent a collection of orders 
   associated with the same symbol (ticker).
   Main use is to get top elements, hide matched orders, etc. */
class OrderMinMaxHeap {
private:
	/* Min max heap data. */
	/* Active, heapified orders are first,
	   then inactive orders (which do not get heapified)
	   inactive orders are those already matched or cancelled.
	   after inactive orders is unused space. 
	   e.g.
	   |AAAAAAAAAAAAAAIIIIIIIIIIIIIIII-----------------| 
	*/
	Order data[MAX_NUM_ORDERS_IN_HEAP];

	Order* active_ptr;
	Order* matched_ptr;

	bool allocated[MAX_NUM_ORDERS_IN_HEAP];

	order_heap_type_t heap_type;

	void swap(
		const int& a, 
		const int& b) {

		Order temp = data[a];
		data[a] = data[b];
		data[b] = temp;
	}

	void make_inactive(const int& i) {
		swap(i, active_ptr - data - 1);
		allocated[active_ptr - data - 1] = false;
		active_ptr--;
		push_down(0);
	}

	bool has_precedence(
		const Order& a,
		const Order& b,
		const time_t& t = -1) const {

		return a.get_price(t) < b.get_price(t);

		bool result = false;
		if (((heap_type == BUY) 
			 && (a.get_price(t) < b.get_price(t))) 
			|| ((heap_type == SELL) 
				&& (a.get_price(t) > b.get_price(t))) 
			|| ((a.get_price(t) == b.get_price(t)) 
				&& (a.get_timestamp(t) <= b.get_timestamp(t)))) {
			result = true;
		}
		return result;
	}

	bool has_precedence(
		const int& a,
		const int& b,
		const time_t& t = -1) const {
		return has_precedence(*(data + a), *(data + b), t);
	}

	bool has_children(const int& a) const {
		return (allocated[a << 1] || allocated[a << 1 + 1]);
	}

	int smallest_descendant(const int& a) const {
		int min = -1;
		if (allocated[a << 1]) {
			min = (*(data + (a << 1))).get_price();
		}
		if (allocated[a << 1 + 1]) {
			if (min == -1 || (*(data + (a << 1))).get_price() < min) {
				min = (*(data + (a << 1))).get_price();
			}
		}
		return min;
	}

	int get_parent(const int& m) const {
		return m << 1;
	}

	bool is_grandchild_of(
		const int& a, 
		const int& b) const {

		bool result = false;
		for (int i = a;
			i > b;
			i = i << 1) {
			result = true;
		}
		return result;
	}

	void push_down_min(const int& m) {
		/* while m has children then :
		i := m
		m := index of the smallest child or
			 grandchild of i */
		for (int i = m, mi = m;
			has_children(mi);
			i = mi, mi = smallest_descendant(mi)) {
			/* if h[m] < h[i] then: */
			if (has_precedence(mi, i)) {
				/* if m is a grandchild of i then: */
				if (is_grandchild_of(mi, i)) {
					/* swap h[m] and h[i] */
					swap(mi, i);
					/* if h[m] > h[parent(m)] then: */
					if (has_precedence(m, get_parent(m))) {
						/* swap h[m] and h[parent(m)] */
						swap(m, get_parent(m));
					}
				}
				else {
					/* swap h[m] and h[i] */
					swap(m, i);
				}
			}
			else {
				/* break */
				break;
			}
		}
	}

	void push_down_max(const int& m) {
		/* while m has children then :
		i := m
		m := index of the smallest child or
			 grandchild of i */
		for (int i = m, mi = m;
			has_children(mi);
			i = mi, mi = smallest_descendant(mi)) {
			/* if h[m] < h[i] then: */
			if (!has_precedence(mi, i)) {
				/* if m is a grandchild of i then: */
				if (is_grandchild_of(mi, i)) {
					/* swap h[m] and h[i] */
					swap(mi, i);
					/* if h[m] > h[parent(m)] then: */
					if (!has_precedence(m, get_parent(m))) {
						/* swap h[m] and h[parent(m)] */
						swap(m, get_parent(m));
					}
				}
				else {
					/* swap h[m] and h[i] */
					swap(m, i);
				}
			}
			else {
				/* break */
				break;
			}
		}
	}

	void push_down(const int& m) {
		/* if i is on a min level then: */
		if (has_children(m)) {
			push_down_min(m);
		}
		else {
			push_down_max(m);
		}
	}

	void push_up_min(const int& m) {
		/* while i has a grandparent and
		   h[i] < h[grandparent(i)] then:*/
		for (int i = m;
			(i << 2) >= 0 && has_precedence(i, i << 2);
			i <<= 2) {
			/*swap h[i] and h[grandparent(i)] */
			swap(i, i << 2);
			/*i : = grandparent(i) */
		}
	}

	void push_up_max(const int& m) {
		/* while i has a grandparent and
		   h[i] < h[grandparent(i)] then:*/
		for (int mi = m;
			(mi << 2) >= 0 && !has_precedence(mi, mi << 2);
			mi <<= 2) {
			/*swap h[i] and h[grandparent(i)] */
			swap(mi, mi << 2);
			/*i : = grandparent(i) */
		}
	}

	void push_up(const int& i) {
		/*  if i is not the root then: */
		if (i != 0) {
			/* if i is on a min level then: */
			if (!has_children(i)) {
				/* if h[i] > h[parent(i)] then: */
				if (has_precedence(i, get_parent(i))) {
					/* swap h[i] and h[parent(i)] */
					swap(i, get_parent(i));
					/* PUSH-UP-MAX(h, parent(i)) */
					push_up_max(get_parent(i));
				}
				else {
					/* PUSH-UP-MIN(h, i) */
					push_up_min(i);
				}
			}
			else {
				/* if h[i] < h[parent(i)] then: */
				if (has_precedence(i, get_parent(i))) {
					/*swap h[i] and h[parent(i)] */
					swap(i, get_parent(i));
					/* PUSH-UP-MIN(h, parent(i)) */
					push_up_min(get_parent(i));
				}
				else {
					/* PUSH-UP-MAX(h, i) */
					push_up_max(i);
				}
			}
		}
	}

	int search(const order_id_t& id) {
		int found_id = -1;
		for (Order* d = data;
			d < matched_ptr;
			d++) {
			if (allocated[d - data]
				&& d->get_order_id() == id) {
				found_id = d - data;
				break;
			}
		}
		return found_id;
	}

public:
	OrderMinMaxHeap(
		const order_heap_type_t& type) {

		for (Order* d = data;
			d < data + MAX_NUM_ORDERS_IN_HEAP;
			d++) {
			*d = Order();
		}
		for (bool* b = allocated;
			b < allocated + MAX_NUM_ORDERS_IN_HEAP;
			b++) {
			*b = false;
		}
		active_ptr = data;
		matched_ptr = active_ptr;
		heap_type = type;
	}

	bool insert(const Order& datum) {
		swap(active_ptr - data, matched_ptr - data);
		/* Append */
		*(active_ptr) = datum;
		allocated[active_ptr - data] = true;
		push_up(active_ptr - data);
		active_ptr++;
		matched_ptr++;
		/* Re-heapify */
		return true;
	}

	bool ammend(
		const order_id_t& id,
		const price_t& new_price,
		const quantity_t& new_quantity) {

		int return_status = ACCEPT;
		int i = search(id);

		if (i != -1) {
			Order* o = data + i;
			o->ammend(new_price, new_quantity);
			o->add_execution_history_entry(
				o->get_history_entry()->history,
				o->get_timestamp(),
				new_price,
				new_quantity);
			push_down(0);
		}
		else {
			return_status = ORDER_DOES_NOT_EXIST;
		}
		return return_status;
	}

	int cancel(const order_id_t& id) {

		int return_status = ACCEPT;
		int i = search(id);

		if (i != -1) {
			Order* o = (data + i);
			(data + i)->add_execution_history_entry(
				CANCELLED,
				o->get_timestamp(),
				o->get_price(),
				o->get_quantity());
			make_inactive(i);
			push_down(0);
		}
		else {
			return_status = ORDER_DOES_NOT_EXIST;
		}
		return return_status;
	}

	Order* get_max_order() {
		Order* max;
		if (!allocated[0]) {
			max = NULL;
		}
		else if (((!allocated[1] || (has_precedence(0, 1))
			&& (!allocated[2] || has_precedence(0, 2))))) {
			max = data;
		}
		else if ((!allocated[2] || has_precedence(1, 2))) {
			max = data + 1;
		}
		else {
			max = data + 2;
		}
		return max;
	}

	Order* get_min_order() {
		return data;
	}

	int get_max_n_orders(
		const time_t& t, 
		Order* output, 
		const int& n) const {
		/* For each element */
		Order *stack = new Order[n];
		int stack_i = 0;
		for (const Order* d = data;
			d < matched_ptr;
			d++) {

			if (!d->active_at(t)) continue;

			/* If it fits in the top n orders so far */
			Order *t_stack = new Order[n];

			/* Meaning we have less than n orders */
			if ((stack_i < n)
				/* Or this is bigger than the smallest */
				|| (has_precedence(*(stack + n - 1), *d, t))) {

				Order* a = stack;
				Order* b = t_stack;
				/* Place until insertion. */
				for (; a <= stack + stack_i; ) {
					if (((a == stack + stack_i) && (stack_i < n))
						|| (has_precedence(*d, *a, t))) {
						*b++ = *d;
						stack_i++;
						break;
					} else {
						*b++ = *a++;
					}  
				}
				/* Place after insertion till finished. */
				for (; b < t_stack + stack_i; ) {
					*b++ = *a++;
				}
			}
			for (int i = 0; i < stack_i; i++) {
				*(stack + i) = *(t_stack + i);
			}
			delete[] t_stack;
		}
		for (int i = 0; i < stack_i; i++) {
			*(output + i) = *(stack + i);
		}
		delete[] stack;
		return stack_i;
	}

	bool transact(
		const order_id_t& local_id, 
		const price_t& deduction, 
		const time_t& t) {
		
		int local_i = search(local_id);
		Order* local = data + local_i;

		int abs_diff = local->get_quantity(t) - deduction;

		if (abs_diff <= 0) {
			local->transact(t, local->get_price(t), 0);
			make_inactive(local_i);
		} else {
			local->transact(
				t,
				local->get_price(t),
				local->get_quantity(t) - deduction);
		}
		return true;
	}
};

/* Class representing a symbol within an orderbook hash table,
   storing the buys and sells for a symbol, alongside the 
   next symbol in the hash table entry. */
class OrderEntry {
private:
	char* symbol;
	OrderMinMaxHeap* buys;
	OrderMinMaxHeap* sells;	
	OrderEntry* next_entry;
public:	
	OrderEntry(char* s)
		: symbol(s),
		buys(new OrderMinMaxHeap(BUY_HEAP)),
		sells(new OrderMinMaxHeap(SELL_HEAP)),
		next_entry(NULL) {
		symbol = s;
	};

	char* get_symbol() const {
		return symbol;
	}

	OrderEntry* get_next_entry() const {
		return next_entry;
	}

	bool add(
		const Order& o, 
		const side_t& s) {

		switch (s) {
			case BUY:
				buys->insert(o);
				break;
			case SELL:
				sells->insert(o);
				break;
			default: break;
		}
		return true;
	}

	bool ammend(
		const side& s, 
		const order_id_t& oid, 
		const price_t& p, 
		const quantity_t& q) {

		switch (s) {
			case BUY: buys->ammend(oid, p, q); break;
			case SELL: sells->ammend(oid, p, q); break;
			default: break;
		}
		return true;
	}

	int cancel(
		const order_id_t& oid, 
		const side& s) {

		switch (s) {
			case BUY: buys->cancel(oid); break;
			case SELL: sells->cancel(oid); break;
			default: break;
		}
		return ACCEPT;
	}

	int match(const time_t &t) {

		while (true) {

			Order* best_buy = buys->get_max_order();
			Order* best_sell = sells->get_min_order();

			if (!best_buy || !best_sell) break;
			/* Collect buy data.  */
			price_t b_price = best_buy->get_price();
			order_id_t b_oi = best_buy->get_order_id();
			quantity_t b_quantity = best_buy->get_quantity();
			order_type_t b_t = best_buy->get_order_type();
		
			/* Collect sell data. */
			price_t s_price = best_sell->get_price();
			order_id_t s_oi = best_sell->get_order_id();
			quantity_t s_quantity = best_sell->get_quantity();
			order_type_t s_t = best_sell->get_order_type();


			if (best_buy->get_history_entry()->history != EXECUTED
				&& best_sell->get_history_entry()->history != EXECUTED
				&& b_price >= s_price) {

				cout << symbol
					 << "|"
					 << b_oi
					 << "," << order_type_to_str[b_t]
					 << "," << b_quantity
					 << "," << b_price
					 << "|" 
					 << s_price
					 << "," << s_quantity
					 << "," << order_type_to_str[s_t]
					 << "," << s_oi
					 << endl;
				
				buys->transact(b_oi, b_price, t);
				sells->transact(s_oi, b_price, t);
			} else {
				break;
			}
		}
		return ACCEPT;
	}

	int query(const time_t& t = -1) {
		
   		Order top_buys[5];
		int num_buys = buys->get_max_n_orders(t, top_buys, 5);

		Order min_sells[5];
		int num_sells = sells->get_max_n_orders(t, min_sells, 5);
		
		int num_to_display = (num_buys > num_sells)
							 ? num_buys
							 : num_sells;

		for (int i = 0;  i < num_to_display; i++) {		
			cout << symbol << "|";

			/* BuyState		*/
			if (i < num_buys) {

				const char* order_type_str = order_type_to_str[
					top_buys[i].get_order_type()
				];

				cout << top_buys[i].get_order_id()
					 << "," << order_type_str
					 << "," << top_buys[i].get_quantity(t)
					 << "," << top_buys[i].get_price(t);
			}
			cout << "|";

			/* SellState	*/
			if (i < num_sells) {

				const char* order_type_str = order_type_to_str[
					min_sells[i].get_order_type()
				];

				cout << min_sells[i].get_price(t)
					 << "," << min_sells[i].get_quantity(t)
					 << "," << order_type_str
					 << "," << min_sells[i].get_order_id();
			}
			cout << endl;
		}
		return ACCEPT;
	}
};

typedef enum command_format_t {
	F_NEW,
	F_AMMEND,
	F_MATCH_TIMESTAMP,
	F_MATCH_TIMESTAMP_SYMBOL,
	F_QUERY,
	F_QUERY_SYMBOL,
	F_QUERY_TIMESTAMP,
	F_QUERY_TIMESTAMP_SYMBOL,
	F_QUERY_SYMBOL_TIMESTAMP
} command_format_t;

/* Class representing a command for the equity order matcher. */
class Command {
public:
	command_format_t format;
	action_t order_action;
	order_id_t order_id;
	time_t timestamp;
	symbol_t symbol;
	side_t side;
	order_type_t order_type;
	price_t price;
	quantity_t quantity;

	Command(
		action_t ao,
		order_id_t oi,
		time_t t,
		symbol_t s,
		side_t si,
		order_type_t ot,
		price_t p,
		quantity_t q)
		: order_action(ao),
		order_id(oi),
		timestamp(t),
		symbol(s),
		side(si),
		order_type(ot),
		price(p),
		quantity(q) {};

	void print() {
		cout << "order_action=" << order_action << ", "
			<< "order_id=" << order_id << ", "
			<< "timestamp=" << timestamp << ", "
			<< "symbol=" << symbol << ", "
			<< "side=" << side << ", "
			<< "order_type=" << order_type << ", "
			<< "price=" << price << ", "
			<< "quantity=" << quantity
			<< endl;
	}
};

typedef struct symbol_name_list {
	symbol_t symbol;
	symbol_name_list* next;
} symbol_name_list;

typedef struct hash_order_to_symbol {
	order_id_t oi;
	char* symbol;
	hash_order_to_symbol* next;
} hash_order_to_symbol;

/* Class representing an order book that stores symbols, orders
   and associated caching information. */
class OrderBook {
private:
	OrderEntry* table[NUM_TOTAL_SYMBOLS_IN_ORDER_BOOK];
	time_t timestamp;

	/* Caching */	
	hash_order_to_symbol* 
		cache_order_to_symbol[NUM_TOTAL_SYMBOLS_IN_ORDER_BOOK];

	symbol_name_list* cache_list_sorted_symbol_names;

	static unsigned int hash(unsigned int x) {
		return ((1279 * x + 2203) 
				% NUM_TOTAL_SYMBOLS_IN_ORDER_BOOK);
	}
	
	static unsigned int hash(char* x) {
		unsigned int int_repr = 0;
		for (const char* xi = x; *xi; xi++) {
			int_repr *= 26;
			int_repr += *xi - 'a';
		}
		return hash(int_repr);
	}

	bool insert(symbol_t key) {
		/* Add symbol to hash table */
		char* symbol = _strdup(key.c_str());
		OrderEntry** lookup_result_addr = table + hash(symbol);
		OrderEntry* lookup_result = *lookup_result_addr;
		*lookup_result_addr = new OrderEntry(symbol);
		/* Add symbol to list cache */
		symbol_name_list* snl = cache_list_sorted_symbol_names;
		if (snl == NULL) {
			cache_list_sorted_symbol_names = new symbol_name_list {
				key, NULL
			};
		} else if ((strcmp(cache_list_sorted_symbol_names->symbol.c_str(),
						   key.c_str()) 
				   >= 0)) {

			symbol_name_list* n_snl = new symbol_name_list{
				key, cache_list_sorted_symbol_names
			};
			cache_list_sorted_symbol_names = n_snl;

		} else {
			while (true) {
				if ((snl->next != NULL)
					&& (strcmp(snl->symbol.c_str(), key.c_str()) < 0)) {
					snl = snl->next; 
				}
				else {
					break;
				}
			}
			symbol_name_list *n_snl = new symbol_name_list {
				key, snl->next
			};
			snl->next = n_snl;
		}
		return true;
	}

	OrderEntry* lookup(const symbol_t& key) {
		char* symbol = _strdup(key.c_str());
		int h_val = hash(symbol);
		OrderEntry* lookup_result = *(table + h_val);
		if (lookup_result) {
			while (lookup_result->get_next_entry()) {
				if (strcmp(lookup_result->get_symbol(), symbol) != 0) {
					lookup_result = lookup_result->get_next_entry();
					continue;
				}
				break;
			}
		}
		delete symbol;
		return lookup_result;
	} 
public:
	OrderBook() {
		for (OrderEntry** t = table;
			t < (table + NUM_TOTAL_SYMBOLS_IN_ORDER_BOOK);
			t++) {
			*t = NULL;
		}
		for (hash_order_to_symbol** s = cache_order_to_symbol;
			s < (cache_order_to_symbol + NUM_TOTAL_SYMBOLS_IN_ORDER_BOOK);
			s++) {
			*s = NULL;
		}
		timestamp = 0;
	}

	bool add_new_order(const Command& o) {

		int status = ACCEPT;
		if (o.timestamp < timestamp) {
			status = INVALID_ORDER_DETAILS;
		} else {
			OrderEntry* oe = lookup(o.symbol);
			if (!oe) {
				insert(o.symbol);
				oe = lookup(o.symbol);
			}
			Order datum = Order(
				o.order_id,
				o.order_type,
				o.timestamp,
				o.price,
				o.quantity);

			/* Record this order in the order-to-symbol cache map */
			int h = hash((unsigned int) o.order_id);
			hash_order_to_symbol **entry = cache_order_to_symbol + h;
			if (*entry == NULL) {
				*entry = new hash_order_to_symbol{
				o.order_id,
				_strdup(o.symbol.c_str()),
				};
			} else {
				hash_order_to_symbol* e;
				for (e = *entry; e->next; e = e->next);
				e->next = new hash_order_to_symbol{
					o.order_id,
					_strdup(o.symbol.c_str()),
				};
			};

			oe->add(datum, o.side);
			timestamp = o.timestamp;
		}
		return status;
	}

	int ammend_order(const Command& o) {
		OrderEntry* oe = lookup(o.symbol);

		int status = ACCEPT;
		if (o.timestamp < timestamp) {
			status = INVALID_ORDER_DETAILS;
		} else if (oe != NULL) {
			status = oe->ammend(
				o.side, 
				o.order_id, 
				o.price, 
				o.quantity);
		} else {		
			status = ORDER_DOES_NOT_EXIST;
		}
		return status;
	}

	int cancel_order(const Command& o) {
		
		OrderEntry* oe = NULL;
		int status = ACCEPT;
		if (o.timestamp < timestamp) {
			status = INVALID_ORDER_DETAILS;
		} else {
			int h = hash((unsigned int) o.order_id);
			char* s = cache_order_to_symbol[h]->symbol;
			if (s && (oe = lookup(s))) {
				status = oe->cancel(o.order_id, o.side);
			} else {
				status = ORDER_DOES_NOT_EXIST;
			}
			timestamp = o.timestamp;
		}
		return status;
	}

	int match(const Command& o) {

		int status = ACCEPT;
		if (o.timestamp < timestamp) {
			status = INVALID_ORDER_DETAILS;

		} else {

			switch (o.format) {

				case F_MATCH_TIMESTAMP:
					/* For each entry in table */
					for (OrderEntry** te = table;
						te < table + NUM_TOTAL_SYMBOLS_IN_ORDER_BOOK;
						te++) {
						/* For each entry in table entry */
						for (OrderEntry* e = *te;
							e != NULL;
							e = e->get_next_entry()) {
							e->match(o.timestamp);
						}
					}
					break;

			case F_MATCH_TIMESTAMP_SYMBOL: {
				OrderEntry* e = lookup(o.symbol);
				e->match(o.timestamp);
				break;
			}

			default:
				break;

			}

			timestamp = o.timestamp;
		}
		return status;
	}

	int query(const Command& o) {

		switch (o.format) {

			case F_QUERY: {
				for (symbol_name_list* snl = cache_list_sorted_symbol_names;
					snl != NULL;
					snl = snl->next) {
				
					OrderEntry* entry = lookup(snl->symbol);
					entry->query();
				}
				break;
			}

			case F_QUERY_SYMBOL: {
				OrderEntry* entry = lookup(o.symbol);
				entry->query();		
				break;
			}				
			
			case F_QUERY_TIMESTAMP: {
				for (symbol_name_list* snl = cache_list_sorted_symbol_names;
					snl != NULL;
					snl = snl->next) {
					
					OrderEntry* entry = lookup(snl->symbol);
					entry->query(o.timestamp);
				}
				break;
			}
			
			case F_QUERY_SYMBOL_TIMESTAMP:
			case F_QUERY_TIMESTAMP_SYMBOL: {
				OrderEntry* entry = lookup(o.symbol);
				entry->query(o.timestamp);
				break;
			}
		} 
		return ACCEPT;
	}
};

#endif