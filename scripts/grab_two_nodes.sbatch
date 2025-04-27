#!/bin/bash
#SBATCH --job-name=two-node-reservation
#SBATCH --nodes=2
#SBATCH --ntasks-per-node=1
#SBATCH --time=5:00:00
#SBATCH --output=reservation_%j.out
#SBATCH --error=reservation_%j.err
#SBATCH --partition=mit_normal  # <-- set if needed

# Print hostnames of the nodes
echo "Reserved nodes:"
scontrol show hostnames $SLURM_JOB_NODELIST

# Keep job alive by sleeping
sleep 18000  # 5 hours

