using namespace QPI;

// Safe math library to prevent overflows
namespace SafeMath {
    uint64 add(uint64 a, uint64 b) {
        uint64 c = a + b;
        REQUIRE(c >= a, "SafeMath: addition overflow");
        return c;
    }

    uint64 sub(uint64 a, uint64 b) {
        REQUIRE(b <= a, "SafeMath: subtraction overflow");
        return a - b;
    }
}

struct HM25 : public ContractBase
{
public:
    struct Echo_input{};
    struct Echo_output{};

    struct Burn_input{};
    struct Burn_output{};

    struct GetStats_input {};
    struct GetStats_output
    {
        uint64 numberOfEchoCalls;
        uint64 numberOfBurnCalls;
        uint64 totalBurned;
    };

    // Added admin functionality
    struct SetAdmin_input { identity newAdmin; };
    struct SetAdmin_output {};

private:
    uint64 numberOfEchoCalls;
    uint64 numberOfBurnCalls;
    uint64 totalBurned;
    identity admin;
    bool locked;  // Reentrancy guard

    // ========== Security Enhancements ========== //
    
    // Modifier for admin-only functions
    #define ONLY_ADMIN \
        REQUIRE(qpi.invocator() == state.admin, "Admin only"); \
        _
    
    // Reentrancy guard modifier
    #define NON_REENTRANT \
        REQUIRE(!state.locked, "Reentrant call"); \
        state.locked = true; \
        _ \
        state.locked = false; \
        _
    
    // ========== Core Functions ========== //
    
    /**
     * Send back the invocation amount with reentrancy protection
     */
    PUBLIC_PROCEDURE(Echo)
        NON_REENTRANT
        
        // Safe state modification
        state.numberOfEchoCalls = SafeMath::add(state.numberOfEchoCalls, 1);
        
        uint64 reward = qpi.invocationReward();
        if (reward > 0) {
            // Validate transfer recipient
            identity invocator = qpi.invocator();
            REQUIRE(invocator != identity{}, "Invalid invocator");
            
            // Safe transfer
            qpi.transfer(invocator, reward);
        }
    _

    /**
     * Burn all invocation amount with admin protection
     */
    PUBLIC_PROCEDURE(Burn)
        ONLY_ADMIN
        NON_REENTRANT
        
        state.numberOfBurnCalls = SafeMath::add(state.numberOfBurnCalls, 1);
        
        uint64 reward = qpi.invocationReward();
        if (reward > 0) {
            // Track burned amount
            state.totalBurned = SafeMath::add(state.totalBurned, reward);
            qpi.burn(reward);
        }
    _

    // ========== Admin Functions ========== //
    
    /**
     * Update contract admin
     */
    PUBLIC_PROCEDURE(SetAdmin)
        ONLY_ADMIN
        NON_REENTRANT
        
        identity newAdmin = input.newAdmin;
        REQUIRE(newAdmin != identity{}, "Invalid admin address");
        REQUIRE(newAdmin != state.admin, "Same admin");
        
        state.admin = newAdmin;
    _

    // ========== View Functions ========== //
    
    PUBLIC_FUNCTION(GetStats)
        output.numberOfBurnCalls = state.numberOfBurnCalls;
        output.numberOfEchoCalls = state.numberOfEchoCalls;
        output.totalBurned = state.totalBurned;
    _

    REGISTER_USER_FUNCTIONS_AND_PROCEDURES
        REGISTER_USER_PROCEDURE(Echo, 1);
        REGISTER_USER_PROCEDURE(Burn, 2);
        REGISTER_USER_PROCEDURE(SetAdmin, 3);  // New admin function
        
        REGISTER_USER_FUNCTION(GetStats, 1);
    _

    INITIALIZE
        state.numberOfEchoCalls = 0;
        state.numberOfBurnCalls = 0;
        state.totalBurned = 0;
        state.admin = qpi.invocator();  // Set deployer as admin
        state.locked = false;           // Reentrancy flag
    _
};
