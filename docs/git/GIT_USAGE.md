# Git Usage Guide - Universal IR Remote

**Version:** 3.3.0
**Last Updated:** December 29, 2025

This guide explains how to use Git for the Universal IR Remote project, including workflows for developers and contributors.

---

## Table of Contents

1. [Repository Setup](#repository-setup)
2. [Branching Strategy](#branching-strategy)
3. [Commit Message Guidelines](#commit-message-guidelines)
4. [Pull Request Process](#pull-request-process)
5. [Release Process](#release-process)
6. [Common Git Tasks](#common-git-tasks)

---

## Repository Setup

### Initial Clone

```bash
# Clone the repository
git clone https://github.com/your-org/Universal_IR_Remote.git
cd Universal_IR_Remote

# Verify repository status
git status
```

### Configure Git

```bash
# Set your identity
git config user.name "Your Name"
git config user.email "your.email@example.com"

# Set default editor (optional)
git config core.editor "vim"  # or "code", "nano", etc.

# Enable color output
git config color.ui auto
```

### Keep Repository Clean

The project includes a comprehensive `.gitignore` that excludes:
- Build artifacts (`build/`)
- SDK configuration (`sdkconfig`, `sdkconfig.old`)
- IDE files (`.vscode/`, `.idea/`)
- Dependencies (`managed_components/`)
- Temporary files (`*.tmp`, `*.bak`)

**Never commit:**
- Build outputs
- Binary files (unless documented)
- Personal configuration files
- Credentials or API keys

---

## Branching Strategy

### Branch Types

**1. `main` Branch**
- Production-ready code
- Always stable and buildable
- Protected (requires PR for changes)
- Tagged with version numbers

**2. Feature Branches**
- Format: `feature/<feature-name>`
- Created for new features
- Merged to `main` via PR

```bash
git checkout -b feature/add-sony-protocol
```

**3. Bugfix Branches**
- Format: `bugfix/<issue-description>`
- Created for bug fixes
- Merged to `main` via PR

```bash
git checkout -b bugfix/ir-learning-timeout
```

**4. Release Branches**
- Format: `release/v<version>`
- Created for release preparation
- Merged to `main` and tagged

```bash
git checkout -b release/v3.4.0
```

**5. Hotfix Branches**
- Format: `hotfix/<issue-description>`
- Created for urgent fixes to `main`
- Merged immediately

```bash
git checkout -b hotfix/critical-nvs-corruption
```

### Branch Workflow

```
main (protected)
  ├── feature/new-protocol ──┐
  ├── feature/web-interface ──┤
  └── bugfix/led-timing ──────┴─→ merge via PR
```

---

## Commit Message Guidelines

### Format

```
<type>(<scope>): <subject>

<body>

<footer>
```

### Types

| Type | Description | Example |
|------|-------------|---------|
| `feat` | New feature | `feat(ir): add Sony SIRC protocol` |
| `fix` | Bug fix | `fix(ac): correct temperature encoding` |
| `docs` | Documentation | `docs: update user guide for v3.3.0` |
| `style` | Code style | `style: fix indentation in ir_control.c` |
| `refactor` | Code refactoring | `refactor(led): simplify color management` |
| `test` | Tests | `test: add unit tests for NEC decoder` |
| `chore` | Build/tools | `chore: update ESP-IDF to v5.5.1` |
| `perf` | Performance | `perf(tx): reduce IR transmission latency` |

### Examples

**Good Commit Messages:**

```
feat(ir): add Panasonic AC protocol support

Implemented Panasonic AC protocol encoder/decoder with full state
support including temperature, mode, fan speed, and swing control.

Tested with Panasonic CS/CU-RU12WKY model.

Closes #42
```

```
fix(partition): correct fctry readonly flag

The fctry partition must be marked readonly when <12KB to pass
ESP-IDF partition table validation.

Updated:
- partitions.csv
- partitions_4MB.csv

Fixes #58
```

**Bad Commit Messages:**

```
❌ Updated files
❌ Fix bug
❌ Changes
❌ WIP
```

### Commit Message Template

Create `.gitmessage` template:

```bash
cat > ~/.gitmessage << 'EOF'
# <type>(<scope>): <subject> (max 50 chars)
# |<----  Using a Maximum Of 50 Characters  ---->|

# Explain why this change is being made
# |<----   Try To Limit Each Line to a Maximum Of 72 Characters   ---->|

# Provide links or keys to any relevant tickets, articles or other resources
# Example: Closes #23

# --- COMMIT END ---
# Type can be:
#    feat     (new feature)
#    fix      (bug fix)
#    refactor (refactoring code)
#    style    (formatting, missing semi colons, etc; no code change)
#    docs     (changes to documentation)
#    test     (adding or refactoring tests; no production code change)
#    chore    (updating build tasks, package manager configs, etc)
#    perf     (performance improvement)
# --------------------
# Remember to:
#   - Capitalize the subject line
#   - Use the imperative mood in the subject line
#   - Do not end the subject line with a period
#   - Separate subject from body with a blank line
#   - Use the body to explain what and why vs. how
#   - Can use multiple lines with "-" for bullet points in body
EOF

git config commit.template ~/.gitmessage
```

---

## Pull Request Process

### 1. Create Feature Branch

```bash
# Update main
git checkout main
git pull origin main

# Create feature branch
git checkout -b feature/my-feature
```

### 2. Make Changes

```bash
# Make your changes
# ... edit files ...

# Check status
git status

# Stage changes
git add main/app_config.h components/ir_control/ir_protocols.c

# Commit with good message
git commit
```

### 3. Push Branch

```bash
# Push to remote
git push origin feature/my-feature
```

### 4. Create Pull Request

**On GitHub:**
1. Navigate to repository
2. Click "New Pull Request"
3. Select your branch
4. Fill out PR template:

```markdown
## Description
Brief description of changes

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Breaking change
- [ ] Documentation update

## Testing
- [ ] Tested on ESP32
- [ ] Tested on ESP32-S3
- [ ] Unit tests pass
- [ ] No memory leaks

## Checklist
- [ ] Code follows project style
- [ ] Documentation updated
- [ ] CHANGELOG.md updated
- [ ] No warnings on build
```

### 5. Code Review

- Address reviewer comments
- Make requested changes:

```bash
# Make changes
git add .
git commit -m "fix: address review comments"
git push origin feature/my-feature
```

### 6. Merge

Once approved:
- Squash and merge (for features)
- Merge commit (for releases)
- Delete branch after merge

---

## Release Process

### Version Numbering

Follow [Semantic Versioning](https://semver.org/):
- **MAJOR.MINOR.PATCH** (e.g., 3.3.0)
- **MAJOR:** Breaking changes
- **MINOR:** New features (backward compatible)
- **PATCH:** Bug fixes

### Creating a Release

**1. Update Version**

```bash
# Update version.txt
echo "3.4.0" > version.txt

# Update CHANGELOG.md
# Add release notes

git add version.txt CHANGELOG.md
git commit -m "chore: bump version to 3.4.0"
```

**2. Create Release Branch**

```bash
git checkout -b release/v3.4.0
```

**3. Final Testing**

```bash
# Build for all targets
idf.py set-target esp32 && idf.py build
idf.py set-target esp32s3 && idf.py build

# Test on hardware
idf.py flash monitor
```

**4. Merge to Main**

```bash
git checkout main
git merge release/v3.4.0
```

**5. Create Tag**

```bash
git tag -a v3.4.0 -m "Release v3.4.0 - <brief description>"
git push origin main --tags
```

**6. Create GitHub Release**

1. Go to GitHub Releases
2. Click "Draft a new release"
3. Select tag v3.4.0
4. Fill in release notes
5. Attach binaries (optional)
6. Publish release

---

## Common Git Tasks

### Update Your Branch

```bash
# Get latest from main
git checkout main
git pull origin main

# Update your feature branch
git checkout feature/my-feature
git rebase main
```

### Undo Last Commit (Keep Changes)

```bash
git reset --soft HEAD~1
```

### Undo Last Commit (Discard Changes)

```bash
git reset --hard HEAD~1
```

### Stash Changes

```bash
# Save work in progress
git stash save "WIP: working on protocol decoder"

# List stashes
git stash list

# Apply stash
git stash pop
```

### View History

```bash
# Compact view
git log --oneline --graph --decorate

# Detailed view
git log --stat

# Specific file
git log -- path/to/file.c
```

### Clean Untracked Files

```bash
# Preview what will be deleted
git clean -n

# Delete untracked files
git clean -f

# Delete untracked files and directories
git clean -fd
```

### Cherry-Pick Commit

```bash
# Pick a specific commit from another branch
git cherry-pick <commit-hash>
```

### Resolve Merge Conflicts

```bash
# During merge conflict
git status  # See conflicted files

# Edit files to resolve conflicts
# ... manual editing ...

# Mark as resolved
git add conflicted_file.c

# Complete merge
git commit
```

### Compare Branches

```bash
# See differences between branches
git diff main..feature/my-feature

# See commits in feature branch not in main
git log main..feature/my-feature
```

---

## Git Hooks

### Pre-Commit Hook

Create `.git/hooks/pre-commit`:

```bash
#!/bin/sh

# Check for trailing whitespace
git diff-index --check --cached HEAD --

# Ensure no build artifacts
if git diff --cached --name-only | grep -q "^build/"; then
    echo "Error: Attempting to commit build artifacts"
    exit 1
fi

# Run style check (optional)
# ./scripts/check_style.sh

exit 0
```

Make executable:
```bash
chmod +x .git/hooks/pre-commit
```

---

## Troubleshooting

### Accidentally Committed to Main

```bash
# If not pushed yet
git reset --soft HEAD~1
git checkout -b feature/my-fix
git commit -m "fix: correct commit"
```

### Large Files Warning

```bash
# If you accidentally committed large files
git rm --cached large_file.bin
echo "large_file.bin" >> .gitignore
git commit --amend
```

### Diverged Branches

```bash
# If local and remote have diverged
git fetch origin
git rebase origin/main
```

---

## Best Practices

✅ **DO:**
- Commit often with meaningful messages
- Keep commits atomic (one logical change per commit)
- Test before committing
- Update documentation with code changes
- Pull before push
- Use feature branches

❌ **DON'T:**
- Commit build artifacts
- Use `git add .` blindly
- Force push to shared branches
- Commit secrets or credentials
- Make commits too large
- Rewrite public history

---

## Quick Reference

```bash
# Common Commands
git status                    # Check status
git add <file>               # Stage file
git commit                   # Commit staged changes
git push                     # Push to remote
git pull                     # Pull from remote
git checkout -b <branch>     # Create and switch branch
git merge <branch>           # Merge branch
git log --oneline           # View history
git diff                    # View changes

# Branching
git branch                   # List branches
git branch -d <branch>       # Delete branch
git checkout <branch>        # Switch branch

# Remote
git remote -v                # List remotes
git fetch origin            # Fetch remote changes
git push origin <branch>    # Push branch to remote
```

---

**For more help:**
- Git Documentation: https://git-scm.com/doc
- GitHub Guides: https://guides.github.com/
- Project Issues: https://github.com/your-repo/issues

---

**Happy Git-ing!** 🎯
