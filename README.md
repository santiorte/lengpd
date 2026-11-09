# Integrated Itinerary and Fare Engine

**Practice 2 – Languages and Paradigms of Computation (2026-2)**  
**Universidad EAFIT – Systems Engineering**

## Integrantes

- Santiago Ortega Hernandez
- Juan Jose Arango Patiño

## Descripción

El programa modela los medios de transporte del Metro del Valle de Aburrá y calcula el cobro de un itinerario según el perfil del pasajero y las reglas de integración.

Se implementan los medios `MetroLine`, `TramLine`, `CableLine`, `TouristCable` y `FeederRoute`, además de los perfiles de pasajero frecuente, estudiante, adulto mayor, persona con discapacidad y usuario ocasional.

## Requisitos

- Compilador compatible con C++17, como `g++`.
- No se necesita ingresar información por consola.

## Compilar y ejecutar

Desde la carpeta donde se encuentra `main.cpp`:

```bash
g++ -std=c++17 -Wall -Wextra -o practice main.cpp
./practice
```

En Windows, después de compilar, se puede ejecutar con:

```powershell
.\practice.exe
```

En OnlineGDB se debe seleccionar C++17, pegar el contenido completo de `main.cpp` y ejecutar el programa.

## Decisiones principales de diseño

- `TransportMode` es una clase abstracta. Sus funciones `baseFare()` y `serviceType()` son virtuales puras, porque cada medio concreto proporciona esos valores.
- `TransportMode` tiene destructor virtual para permitir la destrucción correcta de objetos derivados cuando se administran mediante punteros a la clase base.
- `describe()` centraliza la descripción de los medios y obtiene los datos variables mediante polimorfismo.
- `Passenger::fareOn()` aplica el cobro y evita que la tarifa sea negativa. La función protegida `computeFare()` permite que cada perfil defina su regla; el perfil frecuente conserva el comportamiento base.
- `Network` administra los medios con `std::unique_ptr`. La función `add()` transfiere la propiedad a la red y devuelve una referencia para observar el objeto.
- `chargeItinerary()` ordena una copia del itinerario por minuto y aplica la ventana de integración de 90 minutos, con un máximo de dos transferencias gratuitas.
- Los reportes utilizan algoritmos de la STL (`std::accumulate`, `std::count_if` y `std::for_each`). `namesMatching()` recibe el criterio desde fuera mediante `std::function`.

## Respuestas para preparar la defensa

### 1. ¿Por qué usar una clase abstracta con funciones virtuales puras en lugar de una enumeración y un `switch`?

Porque cada medio de transporte tiene su propio comportamiento y puede proporcionar su tarifa y tipo de servicio por medio de sus propias funciones. La clase abstracta define lo que todos los medios deben ofrecer, mientras que cada clase derivada implementa los detalles. Así, para agregar otro medio no sería necesario modificar una cadena central de condiciones `switch`; bastaría con crear una nueva clase derivada que cumpla la interfaz.

### 2. ¿Por qué el destructor de `TransportMode` debe ser virtual?

Porque la red almacena objetos derivados a través de punteros `std::unique_ptr<TransportMode>`. Cuando se destruye uno de esos objetos mediante un puntero a la clase base, el destructor virtual garantiza que también se ejecute el destructor de la clase derivada. Si se elimina un objeto derivado mediante un puntero a la clase base y el destructor de la base no es virtual, el comportamiento es indefinido.

## Resultados principales de la ejecución

Con el itinerario de prueba incluido en `main.cpp`, los resultados esperados son:

- Total del pasajero frecuente: **$ 20.640**.
- Transferencias gratuitas: **2**.
- Total del estudiante: **$ 17.800**.
- Total del adulto mayor: **$ 19.170**.

