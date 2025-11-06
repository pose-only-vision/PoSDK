// 任务列表交互功能 v2.1 - 修复缓存问题
document.addEventListener('DOMContentLoaded', function () {
    // 初始化任务列表功能
    initTaskList();

    // 添加进度统计
    addProgressTracking();

    // 定期保存状态
    setInterval(saveTaskState, 5000); // 每5秒保存一次
});

function initTaskList() {
    // 查找所有任务列表项
    const taskItems = document.querySelectorAll('.task-list-item');

    taskItems.forEach(function (item, index) {
        const checkbox = item.querySelector('.task-list-item-checkbox');
        if (checkbox) {
            // 设置唯一ID
            const taskId = `task-${getPageId()}-${index}`;
            checkbox.id = taskId;
            checkbox.dataset.taskId = taskId;

            // 优先使用文档中的状态，localStorage作为补充
            // 如果文档中没有预设状态，则从localStorage加载
            if (!checkbox.hasAttribute('checked')) {
                const savedState = localStorage.getItem(taskId);
                if (savedState === 'checked') {
                    checkbox.checked = true;
                    updateTaskAppearance(item, true);
                }
            } else {
                // 文档中有预设状态，确保显示正确
                updateTaskAppearance(item, checkbox.checked);
            }

            // 添加点击事件
            checkbox.addEventListener('change', function () {
                const isChecked = this.checked;
                updateTaskAppearance(item, isChecked);
                saveTaskState();
                updateProgressBar();

                // 添加完成动画效果
                if (isChecked) {
                    item.style.animation = 'none';
                    item.offsetHeight; // 触发重流
                    item.style.animation = 'checkmark 0.3s ease-in-out';
                }
            });

            // 解析任务优先级
            const text = item.textContent;
            if (text.includes('高优先级') || text.includes('🔴')) {
                checkbox.dataset.priority = 'high';
                item.classList.add('high-priority');
            } else if (text.includes('中优先级') || text.includes('🟡')) {
                checkbox.dataset.priority = 'medium';
                item.classList.add('medium-priority');
            } else if (text.includes('低优先级') || text.includes('🟢')) {
                checkbox.dataset.priority = 'low';
                item.classList.add('low-priority');
            }

            // 工时信息已包含在任务描述中，不需要额外显示
        }
    });

    // 初始化进度条
    updateProgressBar();
}

function updateTaskAppearance(item, isChecked) {
    const textElements = item.querySelectorAll('*:not(.task-list-item-checkbox)');

    textElements.forEach(function (element) {
        if (isChecked) {
            element.style.textDecoration = 'line-through';
            element.style.opacity = '0.6';
            element.style.color = '#666';
        } else {
            element.style.textDecoration = 'none';
            element.style.opacity = '1';
            element.style.color = '';
        }
    });

    // 更新优先级背景
    if (isChecked) {
        item.style.backgroundColor = '#f8f9fa';
    } else {
        const priority = item.querySelector('.task-list-item-checkbox')?.dataset.priority;
        switch (priority) {
            case 'high':
                item.style.backgroundColor = '#fff5f5';
                break;
            case 'medium':
                item.style.backgroundColor = '#fffaf0';
                break;
            case 'low':
                item.style.backgroundColor = '#f0fff4';
                break;
            default:
                item.style.backgroundColor = '';
        }
    }
}

function saveTaskState() {
    const checkboxes = document.querySelectorAll('.task-list-item-checkbox');
    checkboxes.forEach(function (checkbox) {
        const taskId = checkbox.dataset.taskId;
        if (taskId) {
            localStorage.setItem(taskId, checkbox.checked ? 'checked' : 'unchecked');
        }
    });
}

function addProgressTracking() {
    // 查找任务列表容器
    const taskContainers = document.querySelectorAll('h3, h4');

    taskContainers.forEach(function (header) {
        if (header.textContent.includes('任务') && !header.textContent.includes('协作')) {
            const nextElement = header.nextElementSibling;
            if (nextElement && (nextElement.tagName === 'UL' || nextElement.tagName === 'OL')) {
                addProgressBarToSection(header, nextElement);
            }
        }
    });
}

function addProgressBarToSection(header, listElement) {
    const checkboxes = listElement.querySelectorAll('.task-list-item-checkbox');
    if (checkboxes.length === 0) return;

    const progressDiv = document.createElement('div');
    progressDiv.className = 'task-progress';
    progressDiv.innerHTML = `
        <div class="progress-info">
            <span class="progress-text">进度: <span class="current">0</span>/<span class="total">${checkboxes.length}</span> 任务</span>
            <span class="progress-percentage">0%</span>
        </div>
        <div class="progress-bar">
            <div class="progress-fill" style="width: 0%;">0%</div>
        </div>
    `;

    // 插入到列表前面
    listElement.parentNode.insertBefore(progressDiv, listElement);

    // 更新这个部分的进度
    updateSectionProgress(progressDiv, checkboxes);
}

function updateProgressBar() {
    // 更新全局进度
    const allCheckboxes = document.querySelectorAll('.task-list-item-checkbox');
    const checkedBoxes = document.querySelectorAll('.task-list-item-checkbox:checked');

    if (allCheckboxes.length === 0) return;

    const percentage = Math.round((checkedBoxes.length / allCheckboxes.length) * 100);

    // 更新各个部分的进度
    document.querySelectorAll('.task-progress').forEach(function (progressDiv) {
        const section = progressDiv.nextElementSibling;
        if (section) {
            const sectionCheckboxes = section.querySelectorAll('.task-list-item-checkbox');
            updateSectionProgress(progressDiv, sectionCheckboxes);
        }
    });

    // 显示完成庆祝效果
    if (percentage === 100 && checkedBoxes.length > 0) {
        showCompletionCelebration();
    }
}

function updateSectionProgress(progressDiv, checkboxes) {
    const total = checkboxes.length;
    const completed = Array.from(checkboxes).filter(cb => cb.checked).length;
    const percentage = total > 0 ? Math.round((completed / total) * 100) : 0;

    const currentSpan = progressDiv.querySelector('.current');
    const percentageSpan = progressDiv.querySelector('.progress-percentage');
    const progressFill = progressDiv.querySelector('.progress-fill');

    if (currentSpan) currentSpan.textContent = completed;
    if (percentageSpan) percentageSpan.textContent = percentage + '%';
    if (progressFill) {
        progressFill.style.width = percentage + '%';
        progressFill.textContent = percentage + '%';

        // 根据进度更改颜色
        if (percentage >= 100) {
            progressFill.style.background = 'linear-gradient(90deg, #2ed573, #7bed9f)';
        } else if (percentage >= 75) {
            progressFill.style.background = 'linear-gradient(90deg, #ffa502, #ff9f43)';
        } else {
            progressFill.style.background = 'linear-gradient(90deg, #007acc, #00a8ff)';
        }
    }
}

function showCompletionCelebration() {
    // 创建庆祝动画
    const celebration = document.createElement('div');
    celebration.innerHTML = '🎉 恭喜！所有任务已完成！🎉';
    celebration.style.cssText = `
        position: fixed;
        top: 50%;
        left: 50%;
        transform: translate(-50%, -50%);
        background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
        color: white;
        padding: 20px 40px;
        border-radius: 15px;
        font-size: 24px;
        font-weight: bold;
        z-index: 10000;
        box-shadow: 0 10px 30px rgba(0,0,0,0.3);
        animation: celebrationPop 3s ease-in-out;
    `;

    document.body.appendChild(celebration);

    // 3秒后移除
    setTimeout(() => {
        celebration.remove();
    }, 3000);
}

function getPageId() {
    // 根据页面URL生成唯一ID
    return window.location.pathname.replace(/[^a-zA-Z0-9]/g, '-');
}

// 添加键盘快捷键支持
document.addEventListener('keydown', function (e) {
    // Ctrl/Cmd + Shift + A: 全选所有任务
    if ((e.ctrlKey || e.metaKey) && e.shiftKey && e.key === 'A') {
        e.preventDefault();
        const checkboxes = document.querySelectorAll('.task-list-item-checkbox');
        checkboxes.forEach(cb => {
            cb.checked = true;
            updateTaskAppearance(cb.closest('.task-list-item'), true);
        });
        saveTaskState();
        updateProgressBar();
    }

    // Ctrl/Cmd + Shift + R: 重置所有任务
    if ((e.ctrlKey || e.metaKey) && e.shiftKey && e.key === 'R') {
        e.preventDefault();
        if (confirm('确定要重置所有任务状态吗？')) {
            const checkboxes = document.querySelectorAll('.task-list-item-checkbox');
            checkboxes.forEach(cb => {
                cb.checked = false;
                updateTaskAppearance(cb.closest('.task-list-item'), false);
            });
            saveTaskState();
            updateProgressBar();
        }
    }
});

// 工具函数
window.taskListUtils = {
    // 导出进度
    exportProgress: function () {
        const data = {
            pageId: getPageId(),
            timestamp: new Date().toISOString(),
            tasks: []
        };

        document.querySelectorAll('.task-list-item-checkbox').forEach(cb => {
            const item = cb.closest('.task-list-item');
            data.tasks.push({
                id: cb.dataset.taskId,
                text: item.textContent.trim(),
                checked: cb.checked,
                priority: cb.dataset.priority || 'normal'
            });
        });

        const blob = new Blob([JSON.stringify(data, null, 2)], { type: 'application/json' });
        const url = URL.createObjectURL(blob);
        const a = document.createElement('a');
        a.href = url;
        a.download = `task-progress-${new Date().toISOString().split('T')[0]}.json`;
        a.click();
        URL.revokeObjectURL(url);
    },

    // 清除所有缓存状态
    clearCache: function () {
        if (confirm('确定要清除所有任务状态缓存吗？页面将重新加载以显示文档的原始状态。')) {
            // 清除当前页面的localStorage
            const pageId = getPageId();
            const keys = Object.keys(localStorage);
            keys.forEach(key => {
                if (key.startsWith(`task-${pageId}-`)) {
                    localStorage.removeItem(key);
                }
            });

            // 重新加载页面
            location.reload();
        }
    },

    // 重置所有任务状态
    resetAllTasks: function () {
        if (confirm('确定要重置所有任务为未完成状态吗？')) {
            const checkboxes = document.querySelectorAll('.task-list-item-checkbox');
            checkboxes.forEach(cb => {
                cb.checked = false;
                updateTaskAppearance(cb.closest('.task-list-item'), false);
            });
            saveTaskState();
            updateProgressBar();
        }
    }
};

// 添加缓存清理按钮到页面
document.addEventListener('DOMContentLoaded', function () {
    // 在任务列表页面添加工具按钮
    if (window.location.pathname.includes('task_list')) {
        addTaskListControls();
    }
});

function addTaskListControls() {
    // 创建控制面板
    const controlPanel = document.createElement('div');
    controlPanel.style.cssText = `
        position: fixed;
        top: 20px;
        right: 20px;
        background: white;
        padding: 15px;
        border-radius: 8px;
        box-shadow: 0 4px 12px rgba(0,0,0,0.15);
        z-index: 1000;
        font-size: 14px;
    `;

    controlPanel.innerHTML = `
        <h4 style="margin: 0 0 10px 0; color: #333;">任务管理工具</h4>
        <button onclick="taskListUtils.clearCache()" style="margin: 5px; padding: 8px 12px; border: 1px solid #007acc; background: #fff; color: #007acc; border-radius: 4px; cursor: pointer;">
            清除缓存
        </button>
        <button onclick="taskListUtils.resetAllTasks()" style="margin: 5px; padding: 8px 12px; border: 1px solid #ff4757; background: #fff; color: #ff4757; border-radius: 4px; cursor: pointer;">
            重置任务
        </button>
        <button onclick="taskListUtils.exportProgress()" style="margin: 5px; padding: 8px 12px; border: 1px solid #2ed573; background: #fff; color: #2ed573; border-radius: 4px; cursor: pointer;">
            导出进度
        </button>
        <div style="margin-top: 10px; font-size: 12px; color: #666;">
            提示: 如果任务状态不正确，<br/>请点击"清除缓存"按钮
        </div>
    `;

    document.body.appendChild(controlPanel);
}
