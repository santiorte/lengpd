/* =========================================================================
   Santiago Ortega Hernandez y Juan Jose Arango Patiño
   =========================================================================
   
   PRACTICE - STARTING CODE (DO NOT CHANGE THE GIVEN SECTIONS)
   Integrated Itinerary and Fare Engine - Medellin Metro System
   Programming Languages and Paradigms (ST0244) - Universidad EAFIT

   Build: g++ -std=c++17 -Wall -Wextra -o practice main.cpp
   Target environment: OnlineGDB, language selector set to C++17

   WHAT YOU RECEIVE
     - the currency formatter, the Leg and ChargedLeg records and the whole
       main() test harness, so that the expected output is reproducible
     - the declarations you must complete, each marked with TODO

   WHAT YOU MUST WRITE
     - PART 1: the TransportMode hierarchy
     - PART 2: the Passenger hierarchy
     - PART 3: chargeItinerary and the Network class
     - PART 4: the four report functions, with STL algorithms only

   The file must compile with ZERO errors and ZERO warnings under
   g++ -std=c++17 -Wall -Wextra. A warning is a defect, not a detail.
   ========================================================================= */

#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <algorithm>
#include <numeric>
#include <map>
#include <functional>
#include <utility>

/* -------------------------------------------------------------------------
   Utility: Colombian peso formatting (3820 -> "$ 3.820")
   ------------------------------------------------------------------------- */
std::string formatCOP(long long value) {
    std::string digits = std::to_string(value);
    std::string out;
    int count = 0;
    for (auto it = digits.rbegin(); it != digits.rend(); ++it) {
        out.push_back(*it);
        if (++count % 3 == 0 && it + 1 != digits.rend()) {
            out.push_back('.');
        }
    }
    std::reverse(out.begin(), out.end());
    return "$ " + out;
}

/* -------------------------------------------------------------------------
   PART 1. Abstract base class of the transport-mode hierarchy
   ------------------------------------------------------------------------- */
class TransportMode {
public:
    TransportMode(std::string name, int stations)
        : name_(std::move(name)), stations_(stations) {}

    virtual ~TransportMode() = default;          // mandatory: deleted through base pointer

    const std::string& name() const { return name_; }
    int stations() const { return stations_; }

    virtual long long baseFare() const = 0;
    virtual std::string serviceType() const = 0;

    virtual bool allowsStudentFlatFare() const { return true; }
    virtual bool allowsIntegration() const { return true; }
    virtual bool opensIntegrationWindow() const { return true; }

    // Non-virtual template method: delegates the variable parts to subclasses.
    std::string describe() const {
        return serviceType() + " " + name_ + " (" +
               std::to_string(stations_) + " stations) - full fare " +
               formatCOP(baseFare());
    }

private:
    std::string name_;
    int stations_;
};

class MetroLine : public TransportMode {
public:
    using TransportMode::TransportMode;
    long long baseFare() const override { return 3820; }
    std::string serviceType() const override { return "Metro line"; }
};

class TramLine : public TransportMode {
public:
    using TransportMode::TransportMode;
    long long baseFare() const override { return 3820; }
    std::string serviceType() const override { return "Tram line"; }
};

class CableLine : public TransportMode {
public:
    using TransportMode::TransportMode;
    long long baseFare() const override { return 3820; }
    std::string serviceType() const override { return "Cable line"; }
};

class TouristCable : public CableLine {
private:
    long long surcharge_;
public:
    TouristCable(std::string name, int stations, long long surcharge)
        : CableLine(std::move(name), stations), surcharge_(surcharge) {}
    long long baseFare() const override {
        return CableLine::baseFare() + surcharge_;
    }
    std::string serviceType() const override { return "Tourist cable"; }
    bool allowsStudentFlatFare() const override { return false; }
    bool allowsIntegration() const override { return false; }
    bool opensIntegrationWindow() const override { return false; }
};

class FeederRoute : public TransportMode {
public:
    using TransportMode::TransportMode;
    long long baseFare() const override { return 0; }
    std::string serviceType() const override { return "Feeder route"; }
    bool opensIntegrationWindow() const override { return false; }
};

/* -------------------------------------------------------------------------
   PART 2. Passenger hierarchy, Non-Virtual Interface idiom
   fareOn() is NOT virtual: it is the template method that clamps the result.
   computeFare() is the customisation point each profile overrides.
   ------------------------------------------------------------------------- */
class Passenger {
public:
    explicit Passenger(std::string name) : name_(std::move(name)) {}
    virtual ~Passenger() = default;

    // Non-Virtual Interface: this public method always clamps the fare.
    long long fareOn(const TransportMode& mode) const {
        return std::max(0LL, computeFare(mode));
    }

    virtual std::string profile() const = 0;

protected:
    virtual long long computeFare(const TransportMode& mode) const {
        return mode.baseFare();
    }

private:
    std::string name_;
};

class FrequentPassenger : public Passenger {
public:
    using Passenger::Passenger;
    std::string profile() const override { return "Civica frequent"; }
};

class StudentPassenger : public Passenger {
public:
    using Passenger::Passenger;
    std::string profile() const override { return "Municipal student"; }
protected:
    long long computeFare(const TransportMode& mode) const override {
        return mode.allowsStudentFlatFare() ? 1600 : Passenger::computeFare(mode);
    }
};

class SeniorPassenger : public Passenger {
public:
    using Passenger::Passenger;
    std::string profile() const override { return "Senior citizen"; }
protected:
    long long computeFare(const TransportMode& mode) const override {
        return Passenger::computeFare(mode) - 490;
    }
};

class DisabilityPassenger : public Passenger {
public:
    using Passenger::Passenger;
    std::string profile() const override { return "Person with disability"; }
protected:
    long long computeFare(const TransportMode& mode) const override {
        return Passenger::computeFare(mode) - 1100;
    }
};

class BearerPassenger : public Passenger {
public:
    using Passenger::Passenger;
    std::string profile() const override { return "Bearer (occasional)"; }
protected:
    long long computeFare(const TransportMode& mode) const override {
        return Passenger::computeFare(mode) + 580;
    }
};

/* -------------------------------------------------------------------------
   PART 3. The itinerary and the integration rule
   ------------------------------------------------------------------------- */
struct Leg {
    int minute;                   // minutes elapsed since the journey started
    const TransportMode* mode;    // non-owning observer pointer
};

struct ChargedLeg {
    int minute;
    std::string modeName;
    std::string serviceType;
    long long charge;
    bool integrated;              // true: free transfer inside the window
};

// La red conserva la propiedad de los transportes mediante unique_ptr.
class Network {
private:
    std::vector<std::unique_ptr<TransportMode>> modes_;
public:
    TransportMode& add(std::unique_ptr<TransportMode> mode) {
        modes_.push_back(std::move(mode));
        return *modes_.back();
    }

    const std::vector<std::unique_ptr<TransportMode>>& modes() const {
        return modes_;
    }

    void printInventory() const {
        std::for_each(modes_.begin(), modes_.end(),
            [](const std::unique_ptr<TransportMode>& mode) {
                std::cout << "- " << mode->describe() << '\n';
            });
    }
};

std::vector<ChargedLeg> chargeItinerary(
    std::vector<Leg> legs, const Passenger& passenger) {
    // Se ordena la copia local para no modificar el itinerario original.
    std::sort(legs.begin(), legs.end(),
        [](const Leg& a, const Leg& b) { return a.minute < b.minute; });

    std::vector<ChargedLeg> charged;
    bool windowActive = false;
    int windowStart = 0;
    int freeTransfersUsed = 0;

    for (const Leg& leg : legs) {
        // Ignora recorridos sin transporte asociado.
        if (leg.mode == nullptr) continue;
        const TransportMode& mode = *leg.mode;
        long long amount = passenger.fareOn(mode);
        bool integrated = false;

        // Solo los transportes que permiten integrar y abrir ventana participan.
        if (mode.allowsIntegration() && mode.opensIntegrationWindow()) {
            const bool insideWindow = windowActive &&
                leg.minute - windowStart < 90;
            if (insideWindow && freeTransfersUsed < 2) {
                amount = 0;
                integrated = true;
                ++freeTransfersUsed;
            } else {
                windowActive = true;
                windowStart = leg.minute;
                freeTransfersUsed = 0;
            }
        }

        charged.push_back({leg.minute, mode.name(), mode.serviceType(),
                           amount, integrated});
    }
    return charged;
}

/* -------------------------------------------------------------------------
   PART 4. Reports built exclusively with STL algorithms and lambdas
   ------------------------------------------------------------------------- */
long long totalPaid(const std::vector<ChargedLeg>& legs) {
    return std::accumulate(legs.begin(), legs.end(), 0LL,
        [](long long total, const ChargedLeg& leg) {
            return total + leg.charge;
        });
}

long long freeTransferCount(const std::vector<ChargedLeg>& legs) {
    return static_cast<long long>(std::count_if(legs.begin(), legs.end(),
        [](const ChargedLeg& leg) { return leg.integrated; }));
}

// Agrupa los cobros por tipo de servicio sin utilizar ciclos explícitos.
std::map<std::string, long long> paidByServiceType(
    const std::vector<ChargedLeg>& legs) {
    std::map<std::string, long long> totals;
    std::for_each(legs.begin(), legs.end(), [&totals](const ChargedLeg& leg) {
        totals[leg.serviceType] += leg.charge;
    });
    return totals;
}

// Aplica el criterio que recibe desde el exterior y devuelve los nombres coincidentes.
std::vector<std::string> namesMatching(
    const Network& network,
    const std::function<bool(const TransportMode&)>& policy) {
    std::vector<std::string> names;
    std::for_each(network.modes().begin(), network.modes().end(),
        [&names, &policy](const std::unique_ptr<TransportMode>& mode) {
            if (policy(*mode)) names.push_back(mode->name());
        });
    return names;
}

// Une los nombres con coma y espacio mediante accumulate.
std::string joined(const std::vector<std::string>& names) {
    return std::accumulate(names.begin(), names.end(), std::string(),
        [](const std::string& result, const std::string& name) {
            return result.empty() ? name : result + ", " + name;
        });
}

/* -------------------------------------------------------------------------
   MAIN
   ------------------------------------------------------------------------- */
int main() {
    std::cout << "============================================================\n";
    std::cout << "   INTEGRATED ITINERARY AND FARE ENGINE - Valle de Aburra\n";
    std::cout << "============================================================\n\n";

    Network network;
    TransportMode& lineA  = network.add(std::make_unique<MetroLine>("A", 21));
    TransportMode& lineB  = network.add(std::make_unique<MetroLine>("B", 7));
    TransportMode& tramTA = network.add(std::make_unique<TramLine>("T-A Ayacucho", 9));
    TransportMode& cableK = network.add(std::make_unique<CableLine>("K Santo Domingo", 4));
    TransportMode& arvi   = network.add(std::make_unique<TouristCable>("L Arvi", 2, 9180));
    TransportMode& feeder = network.add(std::make_unique<FeederRoute>("130 Robledo", 18));

    std::cout << ">> 1. NETWORK INVENTORY (polymorphic describe)\n";
    network.printInventory();
    std::cout << '\n';

    /* Itinerary deliberately given out of chronological order, so that the
       sort step is observable. */
    const std::vector<Leg> itinerary = {
        {45,  &cableK},
        {0,   &feeder},
        {12,  &lineA},
        {150, &lineB},
        {70,  &tramTA},
        {200, &arvi}
    };

    const FrequentPassenger frequent("Diego");
    const StudentPassenger  student("Valentina");
    const SeniorPassenger   senior("Dona Amparo");

    std::cout << ">> 2. FULL FARE PER PROFILE ON METRO LINE A\n";
    std::cout << "  " << frequent.profile() << " pays " << formatCOP(frequent.fareOn(lineA)) << '\n';
    std::cout << "  " << student.profile()  << " pays " << formatCOP(student.fareOn(lineA))  << '\n';
    std::cout << "  " << senior.profile()   << " pays " << formatCOP(senior.fareOn(lineA))   << '\n';
    std::cout << "  " << student.profile()  << " on the tourist cable pays "
              << formatCOP(student.fareOn(arvi)) << " (no flat fare there)\n\n";

    const std::vector<ChargedLeg> charged = chargeItinerary(itinerary, frequent);

    std::cout << ">> 3. CHARGED ITINERARY (frequent profile)\n";
    std::for_each(charged.begin(), charged.end(), [](const ChargedLeg& c) {
        std::cout << "  min " << c.minute << " | " << c.serviceType << " " << c.modeName
                  << " | " << formatCOP(c.charge)
                  << (c.integrated ? "  [free transfer]" : "") << '\n';
    });
    std::cout << '\n';

    std::cout << ">> 4. REPORTS BUILT WITH STL ALGORITHMS\n";
    std::cout << "  accumulate: total paid -> " << formatCOP(totalPaid(charged)) << '\n';
    std::cout << "  count_if: free transfers -> " << freeTransferCount(charged) << '\n';

    const auto byType = paidByServiceType(charged);
    std::for_each(byType.begin(), byType.end(),
        [](const std::pair<const std::string, long long>& entry) {
            std::cout << "  map: paid on " << entry.first << " -> "
                      << formatCOP(entry.second) << '\n';
        });

    const auto firstPaid = std::find_if(charged.begin(), charged.end(),
        [](const ChargedLeg& c) { return c.charge > 0; });
    std::cout << "  find_if: first leg that actually paid -> "
              << (firstPaid == charged.end() ? std::string("none") : firstPaid->modeName) << '\n';

    const std::function<bool(const TransportMode&)> bigModes =
        [](const TransportMode& m) { return m.stations() > 8; };
    std::cout << "  injected policy: modes with more than 8 stations -> "
              << joined(namesMatching(network, bigModes)) << '\n';

    std::cout << "  total for the student on the same itinerary -> "
              << formatCOP(totalPaid(chargeItinerary(itinerary, student))) << '\n';
    std::cout << "  total for the senior on the same itinerary -> "
              << formatCOP(totalPaid(chargeItinerary(itinerary, senior))) << '\n';

    std::cout << "\n============================================================\n";
    std::cout << "  End of demonstration.\n";
    std::cout << "============================================================\n";
    return 0;
}