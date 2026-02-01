"""
Quick smoke test for the RL training stack.

This is not meant to train anything useful.
The goal is simply to verify that:
  - the simulator bindings load
  - the policy runs forward
  - actions can step the environment
  - gradients flow without blowing up
"""

import numpy as np
import torch
import torch.nn as nn
import torch.optim as optim

import sim_bindings


class TinyActorCritic(nn.Module):
    """
    Minimal actor-critic used purely for sanity checks.
    Intentionally small to keep things fast and obvious.
    """
    def __init__(self, obs_dim, act_dim=1):
        super().__init__()

        self.backbone = nn.Sequential(
            nn.Linear(obs_dim, 32),
            nn.Tanh(),
            nn.Linear(32, 32),
            nn.Tanh(),
        )

        self.mu = nn.Linear(32, act_dim)
        self.log_std = nn.Parameter(torch.zeros(act_dim))
        self.value_head = nn.Linear(32, 1)

    def forward(self, obs):
        x = self.backbone(obs)
        mu = self.mu(x)
        std = self.log_std.exp()
        value = self.value_head(x).squeeze(-1)
        return mu, std, value

    def act(self, obs):
        mu, std, value = self.forward(obs)
        dist = torch.distributions.Normal(mu, std)

        raw_action = dist.rsample()
        action = torch.tanh(raw_action)

        # Tanh correction for log-prob
        logp = dist.log_prob(raw_action)
        logp -= torch.log(1.0 - action.pow(2) + 1e-6)
        logp = logp.sum(-1)

        return action, logp, value


def smoke_test():
    print("\n" + "=" * 60)
    print("RL Pipeline Smoke Test".center(60))
    print("=" * 60)

    # --- Environment ---
    print("\nCreating environment...")
    env = sim_bindings.Environment(
        "scenes/fulcrum.json",
        seed=0,
        dt=1 / 240,
        headless=True,
    )

    obs = env.reset()
    obs_dim = len(obs)
    print(f"Environment OK (obs_dim={obs_dim})")

    # --- Policy ---
    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    policy = TinyActorCritic(obs_dim).to(device)
    optimizer = optim.Adam(policy.parameters(), lr=3e-4)

    param_count = sum(p.numel() for p in policy.parameters())
    print(f"Policy initialized on {device} ({param_count:,} params)")

    # --- Single action sanity check ---
    obs_t = torch.tensor(obs, dtype=torch.float32, device=device).unsqueeze(0)
    with torch.no_grad():
        action, logp, value = policy.act(obs_t)

    print(f"Sample action: {action.item(): .4f}")
    print(f"Log prob:      {logp.item(): .4f}")
    print(f"Value estimate:{value.item(): .4f}")

    # --- Step the environment ---
    act = float(np.clip(action.cpu().numpy().squeeze(), -1.0, 1.0).item())
    next_obs, reward, terminated, truncated = env.step(act)

    print("Step OK")
    print(f"  reward: {reward:.4f}")
    print(f"  done:   {terminated or truncated}")

    # --- Short rollout ---
    print("\nRunning short rollout...")
    total_reward = 0.0
    obs = env.reset()

    for _ in range(10):
        obs_t = torch.tensor(obs, dtype=torch.float32, device=device).unsqueeze(0)
        with torch.no_grad():
            action, _, _ = policy.act(obs_t)

        act = float(np.clip(action.cpu().numpy().squeeze(), -1.0, 1.0).item())
        obs, reward, terminated, truncated = env.step(act)
        total_reward += reward

        if terminated or truncated:
            obs = env.reset()

    print(f"Rollout completed (total_reward={total_reward:.4f})")

    # --- Gradient check ---
    print("\nChecking gradients...")
    # Need to do a forward pass WITH gradients enabled
    obs_t = torch.tensor(obs, dtype=torch.float32, device=device).unsqueeze(0)
    action, logp, value = policy.act(obs_t)
    dummy_loss = -value.mean()
    optimizer.zero_grad()
    dummy_loss.backward()
    optimizer.step()
    print("Gradient update OK")

    print("\n" + "=" * 60)
    print("Smoke test passed".center(60))
    print("=" * 60 + "\n")


if __name__ == "__main__":
    smoke_test()
