
#ifndef ORDERINTERPRETER_H
#define ORDERINTERPRETER_H

#include "OrderBook.h"
#include <iomanip>

#define GOT_ORDER 0
#define FINISHED_ORDERS	1

/**
* Responsible for matching an order action
* and storing the result in a given action_t reference.
*/
bool match_order_action(char*& c, action_t& oa) {
	bool produced = false;
	switch (*c) {
		case 'N': oa = NEW; produced = true; c++; break;
		case 'A': oa = AMEND; produced = true; c++; break;
		case 'X': oa = CANCEL; produced = true; c++; break;
		case 'M': oa = MATCH; produced = true; c++; break;
		case 'Q': oa = QUERY; produced = true; c++; break;
		default: break;
	}
	return produced;
}

/**
* Responsible for matching an order id
* and storing the result in a given order_id_t reference.
*/
bool match_order_id(char*& c, order_id_t& o_id) {
	bool produced = false;
	o_id = 0;
	char* backtrack_ptr = c;
	while (true) {
		switch (*c) {
		case '0':
		case '1': case '2': case '3':
		case '4': case '5': case '6':
		case '7': case '8': case '9':
			o_id *= 10;
			o_id += *c - '0';
			c++;
			continue;
		case ',': produced = true;  break;
		default: 
			if (c != backtrack_ptr) {
				produced = true;
			}
			break;
		}
		break;
	}
	return produced;
}

/**
* Responsible for matching a timestamp
* and storing the result in a given time_t reference.
*/
bool match_timestamp(char*& c, time_t& t) {
	bool produced = false;
	t = 0;
	char* backtrack_ptr = c;
	while (true) {
		switch (*c) {
		case '0':
		case '1': case '2': case '3':
		case '4': case '5': case '6':
		case '7': case '8': case '9':
			t *= 10;
			t += *c - '0';
			c++;
			continue;
		case ',': produced = true;  break;
		default: 
			if (c != backtrack_ptr) { 
				produced = true; 
			}
			break;
		}
		break;
	}
	return produced;
}

/**
* Responsible for matching a symbol (ticker)
* and storing the result in a given symbol_t reference.
*/
bool match_symbol(char*& c, symbol_t& st) {

	bool produced = false;

	char sym[4];
	char* s = sym;
	while (true) {
		switch (*c) {
			case 'A': case 'B': case 'C':
			case 'D': case 'E': case 'F':
			case 'G': case 'H': case 'I':
			case 'J': case 'K': case 'L':
			case 'M': case 'N': case 'O':
			case 'P': case 'Q': case 'R':
			case 'S': case 'T': case 'U':
			case 'V': case 'W': case 'X':
			case 'Y': case 'Z':
				*s++ = *c++;
				continue;
			default:
				if (s == sym) {
					produced = false;
					break;
				} else {
					*s = NULL;
					st = symbol_t(sym);
					produced = true;
					break;
				}
		}
		return produced;
	}
}

/**
* Responsible for matching a order type
* and storing the result in a given order_type_t reference.
*/
bool match_order_type(char*& c, order_type_t& ot) {
	bool produced = false;
	switch (*c) {
		case 'M': ot = MARKET; produced = true; c++; break;
		case 'L': ot = LIMIT; produced = true; c++; break;
		case 'I': ot = IOC; produced = true; c++; break;
		default: break;
	}
	return produced;
}

/**
* Responsible for matching a side
* and storing the result in a given side_t reference.
*/
bool match_side(char*& c, side_t& s) {
	bool produced = false;
	switch (*c) {
		case 'B': s = BUY; produced = true; c++; break;
		case 'S': s = SELL; produced = true; c++; break;
		default: break;
	}
	return produced;
}

/**
* Responsible for matching a price
* and storing the result in a given price_t reference.
*/
bool match_price(char*& c, price_t& q) {
	bool produced = false;

	char* backtrack_ptr = c;

	enum state {
		ENCOUNTERED_,
		ENCOUNTERED_DIGIT_DOT,
		ENCOUNTERED_DIGIT_DOT_DIGIT
	};

	state s = ENCOUNTERED_;

	char lexeme[16];
	char* l = lexeme;

	while (true) {
		switch (s) {

			case ENCOUNTERED_:
				switch (*c) {
				case '0':
				case '1': case '2': case '3':
				case '4': case '5': case '6':
				case '7': case '8': case '9':
					*l++ = *c++;
					continue;;
				case '.':
					s = ENCOUNTERED_DIGIT_DOT;
					*l++ = *c++;
					continue;
				case ',':
					s = ENCOUNTERED_DIGIT_DOT_DIGIT;
					continue;
				default:
					c = backtrack_ptr;
					break;
				}
				break;

			case ENCOUNTERED_DIGIT_DOT:
				switch (*c) {
				case '0':
				case '1': case '2': case '3':
				case '4': case '5': case '6':
				case '7': case '8': case '9':
					*l++ = *c++;
					continue;
				case ',':
					s = ENCOUNTERED_DIGIT_DOT_DIGIT;
					continue;
				default:
					c = backtrack_ptr;
					break;
				}
				break;

			case ENCOUNTERED_DIGIT_DOT_DIGIT:
				*l = NULL;
				q = stof(lexeme, NULL);
				produced = true;
				break;

			default: break;
		}
		break;
	}
	return produced;
}

/**
* Responsible for matching a quantity
* and storing the result in a given quantity_t reference.
*/
bool match_quantity(char*& c, quantity_t& q) {
	bool produced = false;
	q = 0;
	char* backtrack_ptr = c;
	while (true) {
		switch (*c) {
			case '0':
			case '1': case '2': case '3':
			case '4': case '5': case '6':
			case '7': case '8': case '9':
				q *= 10;
				q += *c - '0';
				c++;
				produced = true;
				continue;
			case ',': produced = true;  break;
			default: break;
		}
		break;
	}
	return produced;
}

/**
* Responsible for matching a command
* and storing the result in a given Command reference.
*/
bool match_command(char*& c, Command& o) {

	action_t order_action;
	order_id_t order_id;
	time_t timestamp;
	symbol_t symbol;
	side_t side;
	order_type_t order_type;
	price_t price;
	quantity_t quantity;
	command_format_t format;

	if (*c == '\n') c++;
	match_order_action(c, order_action);
	
	if (*c == ',') c++;

	int history = 0;
	switch (order_action) {
	case NEW:
		if ((match_order_id(c, order_id))
			&& (*c++ == ',')
			&& (match_timestamp(c, timestamp))
			&& (*c++ == ',')
			&& (match_symbol(c, symbol))
			&& (*c++ == ',')
			&& (match_order_type(c, order_type))
			&& (*c++ == ',')
			&& (match_side(c, side))
			&& (*c++ == ',')
			&& (match_price(c, price))
			&& (*c++ == ',')
			&& (match_quantity(c, quantity))) {

			o = Command(order_action,
				order_id,
				timestamp,
				symbol,
				side,
				order_type,
				price,
				quantity);
			history = GOT_ORDER;
			break;
		}
		history = INVALID_ORDER_DETAILS;
		break;

	case AMEND:
		if ((match_order_id(c, order_id))
			&& (*c++ == ',')
			&& (match_timestamp(c, timestamp))
			&& (*c++ == ',')
			&& (match_symbol(c, symbol))
			&& (*c++ == ',')
			&& (match_order_type(c, order_type))
			&& (*c++ == ',')
			&& (match_side(c, side))
			&& (*c++ == ',')
			&& (match_price(c, price))
			&& (*c++ == ',')
			&& (match_quantity(c, quantity))) {

			o = Command(order_action,
				order_id,
				timestamp,
				symbol,
				side,
				order_type,
				price,
				quantity);
			history = GOT_ORDER;
			break;
		}
		history = INVALID_ORDER_DETAILS;
		break;

	case CANCEL:
		if ((match_order_id(c, order_id))
			&& (*c++ == ',')
			&& (match_timestamp(c, timestamp))) {

			o = Command(order_action,
				order_id,
				timestamp,
				symbol_t(""),
				BUY,
				LIMIT,
				(price_t)0,
				(quantity_t)0);
			history = GOT_ORDER;
			break;
		}
		history = INVALID_ORDER_DETAILS;
		break;

	case MATCH: {
		if (match_timestamp(c, timestamp)) {

			if (*c == ',') {
				c++;
				if (match_symbol(c, symbol)) {
					format = F_MATCH_TIMESTAMP_SYMBOL;
				}
			}
			else {
				format = F_MATCH_TIMESTAMP;
			}

			o = Command(order_action,
				0,
				timestamp,
				symbol,
				BUY,
				LIMIT,
				(price_t)0,
				(quantity_t)0);
			o.format = format;
			history = GOT_ORDER;
			break;
		}
		history = INVALID_ORDER_DETAILS;
		break;
	}

	case QUERY:

		switch (*c) {
			case 'A': case 'B': case 'C':
			case 'D': case 'E': case 'F':
			case 'G': case 'H': case 'I':
			case 'J': case 'K': case 'L':
			case 'M': case 'N': case 'O':
			case 'P': case 'Q': case 'R':
			case 'S': case 'T': case 'U':
			case 'V': case 'W': case 'X':
			case 'Y': case 'Z':
				match_symbol(c, symbol);
				if ((*c++ == ',')
					&& (match_timestamp(c, timestamp))) {
					format = F_QUERY_SYMBOL_TIMESTAMP;
				}
				else {
					timestamp = 0;
					format = F_QUERY_SYMBOL;
				}
				break;

			case '0':
			case '1': case '2': case '3':
			case '4': case '5': case '6':
			case '7': case '8': case '9':
				match_timestamp(c, timestamp);
				if ((*c++ == ',')
					&& (match_symbol(c, symbol))) {
					format = F_QUERY_TIMESTAMP_SYMBOL;
				}
				else {
					format = F_QUERY_TIMESTAMP;
					symbol = symbol_t("");
				}
				break;

			default:
				symbol = symbol_t("");
				timestamp = 0;
				format = F_QUERY;
				break;
		}

		o = Command(order_action,
					(order_id_t) 0,
					timestamp,
					symbol,
					BUY,
					LIMIT,
					(price_t) 0,
					(quantity_t) 0);
		o.format = format;
		history = GOT_ORDER;
		break;

	default:
		history = FINISHED_ORDERS;
		break;
	}
	return history;
}

/**
* Responsible for interpreting a command into a given
* OrderBook, altering the state of said orderbook by
* doing so.
*/
bool interpret_order(OrderBook& ob, const Command& o) {
	switch (o.order_action) {

		case NEW: {
			int history = ob.add_new_order(o);
			switch (history) {
				case ACCEPT:
					cout << o.order_id
						 << " - "
						 << "Accept"
						 << endl;
					break;
				case INVALID_ORDER_DETAILS:
					cout << o.order_id
						 << " - "
						 << "Reject"
						 << " - "
						 << INVALID_ORDER_DETAILS
						 << " - "
						 << "Invalid order details"
						 << endl;
					break;
				default:
					break;
			}
			break;
		}

		case AMEND: {
			int history = ob.ammend_order(o);

			switch (history) {
				case ACCEPT:
					cout << o.order_id
						 << " - "
						 << "AmmendAccept"
						 << endl;
					break;
				case INVALID_AMMENDMENT_DETAILS:
					cout << o.order_id
						 << " - "
						 << "AmmendReject"
						 << " - "
						 << INVALID_AMMENDMENT_DETAILS
						 << " - "
						 << "Invalid amendment details"
						 << endl;
					break;
				case ORDER_DOES_NOT_EXIST:
					cout << o.order_id
				 		 << " - "
						 << "AmmendReject"
						 << " - "
						 << ORDER_DOES_NOT_EXIST
						 << " - "
						 << "Order does not exist"
						 << endl;
					break;
				default:
					break;
			}
			break;
		}

		case CANCEL: {
			int history = ob.cancel_order(o);
			switch (history) {
				case ACCEPT:
					cout << o.order_id
						<< " - "
						<< "CancelAccept"
						<< endl;
					break;
				case ORDER_DOES_NOT_EXIST:
					cout << o.order_id
						<< " - "
						<< "CancelReject"
						<< " - "
						<< ORDER_DOES_NOT_EXIST
						<< " - "
						<< "Order does not exist"
						<< endl;
					break;
				default:
					break;
			}
			break;
		}

		case MATCH: {
			ob.match(o);
			break;
		}

		case QUERY: {
			ob.query(o);
			break;
		}
	}
	return true;
}

/**
* Responsible for interpreting commands into a given
* OrderBook, altering the state of said orderbook by
* doing so.
* @param num_orders the num of orders to interpret.
*/
bool interpret_orders(
	char*& c, 
	OrderBook& o, 
	int num_orders = -1) {

	/* Temporary order. */
	Command order = Command(
		NEW, (order_id_t)0, (time_t) 0, symbol_t(""),
		BUY, LIMIT, (price_t)0, (quantity_t)0);


	bool in_process = true;
	int orders_processed = 0;
	while (in_process) {

		int parser_status = match_command(c, order);

		 /*cout << "interpret_orders: recieved ";
		order.print(); */

		switch (parser_status) {

			case INVALID_ORDER_DETAILS: {
				cout << order.order_id
					 << " - Reject - "
					 << INVALID_ORDER_DETAILS
					 << " - "
					 << "Invalid order details"
					 << endl;
				in_process = false;
				break;
			}

			case GOT_ORDER: {
				interpret_order(o, order);
				orders_processed++;
				break;
			}

			case FINISHED_ORDERS:
				break;
		}
		if (((num_orders != -1) 
			&& (orders_processed >= num_orders))
			|| parser_status != GOT_ORDER) break;
		
	}
	return true;
}

/**
* Responsible for matching a number of orders into 
* a given int.
*/
bool match_num_orders(char*& c, int& n) {
	bool produced = false;
	n = 0;
	while (true) {
		switch (*c) {
			case '0':
			case '1': case '2': case '3':
			case '4': case '5': case '6':
			case '7': case '8': case '9':
				n *= 10;
				n += *c - '0';
				c++;
				produced = true;
				continue;
			case ',': produced = true;  break;
			default: break;
		}
		break;
	}
	return produced;
}

void main() {
	char* input1 = new char[] {
		"12\n"
		"N,1,0000001,AB,L,B,104.53,100\n"
		"N,2,0000002,AB,L,S,105.53,100\n"
		"N,3,0000003,AB,L,B,104.53,90\n"
		"M,0000004\n"
		"N,4,0000005,AB,L,S,104.43,80\n"
		"A,2,0000006,AB,L,S,104.42,100\n"
		"Q\n"
		"M,0000008\n"
		"N,5,0000009,AB,L,S,105.53,120\n"
		"X,3,0000010\n"
		"N,6,0000011,XYZ,L,B,1214.82,2568\n"
		"Q"
	};
	char* input2 = new char[] {
		"12\n"
		"N,1,0000001,ALN,L,B,60.90,100\n"
		"N,13,0000002,ALN,L,B,60.90,100\n"
		"N,10,0000003,ALN,L,S,60.90,100\n"  
		"N,12,0000004,ALN,L,S,60.90,100\n"
		"N,11,0000005,ALB,L,S,60.90,100\n" 
		"N,14,0000006,ALB,L,S,62.90,101\n"
		"N,16,0000007,ALB,L,S,63.90,102\n"
		"N,18,0000008,ALB,L,S,64.90,103\n"
		"N,20,0000009,ALB,L,S,65.90,104\n"
		"Q,0000003\n"
		"Q,ALB\n"
		"Q,ALN,0000002\n"
		"Q,0000002,ALN\n"
		"Q"
	};
	cout << "  ";
	OrderBook sys_order_book = OrderBook();
	char* c_ptr = input2;
	int num_orders;
	match_num_orders(c_ptr, num_orders);
	if (*c_ptr == '\n') c_ptr++;
	cout << fixed << setprecision(2);
	interpret_orders(c_ptr, sys_order_book);
}

#endif