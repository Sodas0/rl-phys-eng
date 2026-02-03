#!/usr/bin/env python3
"""
Load and run a trained Stable Baselines 3 PPO policy.
"""

import numpy as np
import time
import gymnasium as gym
from gymnasium import spaces
from stable_baselines3 import PPO
import sim_bindings


# ============================================================
# Gymnasium Environment Wrapper (matches training)
# ============================================================

class BalanceBeamEnv(gym.Env):
    """
    Gymnasium environment for beam-ball balancing task.
    Must match the training environment exactly.
    """
    
    metadata = {'render_modes': ['human'], 'render_fps': 240}
    
    def __init__(self, scene_path, seed=0, dt=1/240, headless=True, random_init=False):
        super().__init__()
        
        self.scene_path = scene_path
        self.dt = dt
        self.headless = headless
        self.random_init = random_init
        self.rng = np.random.RandomState(seed)
        self._seed = seed
        
        # Initialize environment
        self.env = sim_bindings.Environment(scene_path, seed, dt, headless=headless)
        self.max_steps = 2000
        self.steps = 0
        
        # Get observation dimensions
        obs = self.env.reset()
        self.obs_dim = len(obs)
        
        # Observation normalization statistics (has to match training)
        self.obs_scale = np.array([1.0, 0.5, 0.005, 0.01], dtype=np.float32)
        
        # Define action and observation spaces
        self.action_space = spaces.Box(
            low=-1.0, high=1.0, shape=(1,), dtype=np.float32
        )
        
        self.observation_space = spaces.Box(
            low=-np.inf, high=np.inf, shape=(self.obs_dim,), dtype=np.float32
        )
    
    def reset(self, seed=None, options=None):
        """Reset environment to initial state."""
        if seed is not None:
            self._seed = seed
            self.rng = np.random.RandomState(seed)
        
        self.steps = 0
        
        # Use random seed for each episode if random_init is enabled
        if self.random_init:
            new_seed = self.rng.randint(0, 2**31 - 1)
            self.env = sim_bindings.Environment(
                self.scene_path, new_seed, self.dt, headless=self.headless
            )
        
        obs = np.array(self.env.reset(), dtype=np.float32)
        obs = obs * self.obs_scale
        
        return obs, {}
    
    def step(self, action):
        """Execute one step in the environment."""
        if isinstance(action, np.ndarray):
            action = float(action[0])
        else:
            action = float(action)
        
        action = np.clip(action, -1.0, 1.0)
        
        obs, reward, terminated, truncated = self.env.step(action)
        self.steps += 1
        
        if self.steps >= self.max_steps:
            truncated = True
        
        obs = np.array(obs, dtype=np.float32) * self.obs_scale
        
        return obs, reward, terminated, truncated, {}
    
    def render(self):
        """Render the environment."""
        if not self.headless:
            self.env.render()
    
    def close(self):
        """Clean up resources."""
        pass


# ============================================================
# Run Policy
# ============================================================

def run_policy(scene_path, model_path=None, episodes=5, render=True, seed=42):
    """
    Run a trained Stable Baselines 3 policy (or random policy if no model provided).
    
    Args:
        scene_path: Path to scene JSON file
        model_path: Path to saved SB3 model (e.g., "ppo_balance_model.zip")
        episodes: Number of episodes to run
        render: Whether to render visualization
        seed: Random seed
    """
    # Create environment (headless=False so SDL window is created when rendering)
    headless = not render
    if render:
        print("Rendering enabled")
    env = BalanceBeamEnv(
        scene_path=scene_path,
        seed=seed,
        headless=headless,
        random_init=True  
    )
    
    # Load policy if provided
    if model_path:
        print(f"Loading Stable Baselines 3 model from: {model_path}")
        model = PPO.load(model_path)
        print("Model loaded successfully!")
    else:
        print("No model provided - using random policy")
        model = None
    
    print(f"\nRunning {episodes} episodes...")
    print("=" * 60)
    
    episode_returns = []
    episode_lengths = []
    
    for ep in range(episodes):
        obs, _ = env.reset()
        done = False
        total_reward = 0
        steps = 0

        # Draw first frame so window appears before stepping
        if render:
            env.render()
            time.sleep(0.1)

        while not done:
            if model:
                # Use trained policy (deterministic for evaluation)
                action, _ = model.predict(obs, deterministic=True)
            else:
                # Random policy
                action = env.action_space.sample()

            obs, reward, terminated, truncated, _ = env.step(action)
            done = terminated or truncated
            total_reward += reward
            steps += 1

            if render:
                env.render()
                time.sleep(1 / 240)
        
        episode_returns.append(total_reward)
        episode_lengths.append(steps)
        
        print(f"Episode {ep+1:2d}: Length={steps:4d}  Return={total_reward:8.2f}")
    
    print("=" * 60)
    print(f"Mean return: {np.mean(episode_returns):8.2f} ± {np.std(episode_returns):.2f}")
    print(f"Mean length: {np.mean(episode_lengths):8.1f}")
    print(f"Max return:  {np.max(episode_returns):8.2f}")
    print("=" * 60)


# ============================================================
# Main
# ============================================================

def main():
    import argparse
    
    parser = argparse.ArgumentParser(description="Run trained Stable Baselines 3 PPO policy")
    parser.add_argument("--scene", type=str, default="scenes/fulcrum.json",
                        help="Path to scene file")
    parser.add_argument("--model", type=str, default=None,
                        help="Path to saved SB3 model (e.g., ppo_balance_model.zip)")
    parser.add_argument("--episodes", type=int, default=5,
                        help="Number of episodes to run")
    parser.add_argument("--no-render", action="store_true",
                        help="Disable rendering")
    parser.add_argument("--seed", type=int, default=42,
                        help="Random seed")
    
    args = parser.parse_args()
    
    run_policy(
        scene_path=args.scene,
        model_path=args.model,
        episodes=args.episodes,
        render=not args.no_render,
        seed=args.seed
    )


if __name__ == "__main__":
    main()
