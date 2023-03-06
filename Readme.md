**pg_gpt** is a postgres extension that uses OpenAI's GPT-3 API to generate SQL queries from natural language. It can also explain SQL queries and query plans in natural language.

## Installation

#### Clone the repository

`git clone https://github.com/prabin-acharya/pg_gpt`  
`cd pg_gpt`

#### Install dependencies

- postgresql, postgresql-server-dev-xx
- gcc
- libcurl

#### Set OPENAI API Key

Set your secret OPENAI API key in the file `secrets.h`

#### Compile the extension:

`make`  
 `sudo make install`

#### Install

Enter the postgres shell: `psql`  
 Connect to the database you want to install the extension in: `\c demo`  
 Install extension: `create extension pg_gpt;`

Voila!. Now, you can execute any of the following functions:

### Functions

#### gpt_query(text, boolean)

generates a query and optionally executes it

eg. select gpt_query('list all passengers on flight no. PG0134');  
 => `SELECT * FROM passengers WHERE flight_no = 'PG0134';`

eg. select gpt_query(' list passenger name, contact data, booking date and ticket amount for all tickets that were booked for a total amount greater than $500');  
 => `SELECT passenger_name, contact_data, book_date, total_amount from tickets JOIN bookings ON tickets.book_ref = bookings.book_ref WHERE total_amount >500;`

eg. select gpt_query('update seat no 2A of aircraft 319 to Economy', true);  
 => `UPDATE seats SET fare_conditions='Economy' WHERE seat_no='2A' AND aircraft_code='319';`

#### gpt_explain(text)

explains query in natural language

eg. SELECT gpt_explain('SELECT f.departure_airport, COUNT(\*) AS num_flights FROM flights f WHERE f.status=''On Time'' GROUP BY f.departure_airport');  
 => `"This query counts the number of flights that departed on time from each departure airport."`

#### gpt_explain_plan(text)

explains query plan in natural language

eg. select gpt_explain_plan('SELECT f.departure_airport, COUNT(\*) AS num_flights FROM flights f WHERE f.status=' 'On Time' ' GROUP BY f.departure_airport;');  
 => `"The database engine will start by scanning through the flights table to select all rows with a status of 'On Time'. It will then group the results by departure airport and count the number of flights for each airport. Finally it will return the departure airport and the corresponding count of flights."`
