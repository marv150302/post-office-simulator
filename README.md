# Post Office Simulation
## Description

A simulation system that models the behavior of a postal office, handling user arrivals, service queues, ticket distribution, and operator management. It uses processes, shared memory, semaphores, and message queues to simulate concurrency and synchronization.

This project was developed as part of the Operating Systems course assignment at Unito.


## Features

- Dynamic user and operator generation via processes.
- Shared memory for system state (e.g., queues, statistics).
- Ticket system with request and response via message queues.
- Day simulation with adjustable duration and granularity.
- Termination modes:
    - Timeout (after `SIM_DURATION` days).
    - Explosion (user queue exceeds `EXPLODE_THRESHOLD`).
- CSV generation of daily statistics.


---

### 🚀 5. **How to Run**

```markdown
## Running the Simulation

1. Clone the repo:
   git clone https://github.com/marv150302/post-office-simulator/
   cd post-office-simulator
   
2. Build the project: make

3. Run the simulation: make run

4. Add users manually during runtime: ./bin/add_users 10

5. Output statistics will be saved in: ./csv_logs/statistiche.csv

```

---

### ⚙️ 6. **Configuration**
Describe the config file and how parameters affect the simulation.

```markdown
## Configuration

Edit `config/timeout.conf` or `config/explode.conf` to control parameters like:

- `SIM_DURATION`: Number of days to simulate
- `EXPLODE_THRESHOLD`: Max users waiting before triggering shutdown
- `P_SERV_MIN`, `P_SERV_MAX`: Probability ranges for user participation
- `BREAK_PROBABILITY`: Chance an operator takes a break

```

## Statistics Output

At the end of each simulated day, statistics are printed and saved to CSV

## Authors

- Marvel Asuenimhen – [@github](https://github.com/marv150302)