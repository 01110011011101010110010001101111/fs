#!/bin/bash
#SBATCH --job-name=rdma-test
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=1
#SBATCH --cpus-per-task=1
#SBATCH --time=00:05:00
#SBATCH --partition=mit_normal    # <-- replace with your partition if needed
#SBATCH --output=rdma_test_%j.out
#SBATCH --error=rdma_test_%j.err

# module load rdma-core    # <-- or whatever module loads ibv_rc_pingpong on your system
# module load infiniband-diags  # sometimes needed

# Grab the hostnames
hosts=($(scontrol show hostnames $SLURM_JOB_NODELIST))
server_host=${hosts[0]}
client_host=${hosts[1]}

echo "Server: $server_host"
echo "Client: $client_host"

# Start server on the first node
srun --nodes=1 --ntasks=1 --nodelist=$server_host ibv_rc_pingpong &

# Give it a second to start
sleep 2

# Start client on the second node
srun --nodes=1 --ntasks=1 --nodelist=$client_host ibv_rc_pingpong $server_host

wait
