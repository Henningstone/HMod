Contents
-------
- [Prependix](#prependix)
    - [Definition style](#definition-style)
    - [Data accessing](#data-accessing)
- [IServer](#iserver)
    - [Tick](#tick)
    - [TickSpeed](#tickspeed)
    - [MaxClients](#maxclients)
    - [ClientName](#clientname)
    - [ClientClan](#clientclan)
    - [ClientCountry](#clientcountry)
    
# Prependix
## Definition style
Definitions in this documentation are denoted in a cpp-like fashion to indicate the types
the api is using for the respective data or calls.

Since lua only knows the 'number' type for numerical values but the api differentiates between
'int' and 'float', this documentation will do so either. Both refer to the 'number' datatype.
Passing anything but whole numbers to 'int' will be syntactically valid, but will get truncated
and thus might produce unexpected results.

## Data accessing
The api is designed in an object-oriented way using lua tables to provide a clean structure
that represents the native code structure most closely. When using the api you must stick
to a certain protocol closely in order for your scripts to work properly:

- **[data]**
    - this variable is accessed using standard lua table referencing syntax,
      which means `Table.Variable` or `Table["Variable"]`
    - gives raw access to internal native-code class attribute
    - data can be freely read from and written to at any time
    
- **[<readonly/readwrite> property]**
    - syntactically equal to data
    - important difference to data: data exchange with properties is proxied through function
      calls (getters and setters) and might possibly have side-effects. If so, they will
      be denoted in this documentation.
    - **about modifiers:**
        - `readonly`: assigning to this property will result in error
        - `readwrite`: this property can be assigned a value of one specific type

- **functions (methods)**
    - functions in this doc are identified by the syntax of variable name and brackets behind it 
    - calling high-level functions of the api that are part of a class is especially error-prone,
      as you have to use a colon (`:`) instead of period (`.`) to reference it on the table,
      so use `ClassTable:Method(Arg1, Arg2)`, *not* `ClassTable.Method(Args)` as it will result in error!
      

# IServer
## Tick
```cs
[readonly property]
int Tick
```
The current server tick

## TickSpeed
```cs
[readonly property]
int TickSpeed
```
The number of ticks per second the server is running at

## MaxClients
```cs
[readonly property]
int MaxClients
```
How many players the netserver can hold

## ClientName
```cs
string ClientName(int ClientID)
```
Returns the name of the player with ID `ClientID`

## ClientClan
```cs
string ClientClan(int ClientID)
```
Returns the clan of the player with ID `ClientID`

## ClientCountry
```cs
int ClientCountry(int ClientID)
```
Returns the country id of the player with ID `ClientID`

## ClientIngame
```cs
boolean ClientIngame(int ClientID)
```
Returns true if there is a client with the given ID on the server, otherwise false

## 
```cs

```
desc

## 
```cs

```
desc
