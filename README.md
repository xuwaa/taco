# taco
五维度转置放在apps/tensor_5/tensor_transpose.cpp中

当中transpose_coo_21043_k5是优化的计数排序加桶排序算法
项目将transpose_coo按是否完全使用桶排序加计数来排列，分为k0~k5，将120种五维顺序排列了720个情况
**例如：transpose_coo_10324_k0代表直接使用快速排序
**		transpose_coo_10324_k1代表对1使用计数排序，然后对1维度形成桶，然后对剩下的0324进行快速排序
**		transpose_coo_10324_k2代表对1使用计数排序，然后对10维度形成桶，然后对剩下的324进行快速排序
**		transpose_coo_10324_k3代表对1使用计数排序，然后对10维度形成桶，然后对3维度进行桶排序排序好103，再剩下的24进行快速排序
**		transpose_coo_10324_k4代表对1使用计数排序，然后对10维度形成桶，然后对3维度进行桶排序排序好103，再剩下的4进行快速排序
**		transpose_coo_10324_k5代表对1使用计数排序，然后对10维度形成桶，然后对3维度进行桶排序排序好103，再剩下的24进行因为初始为01234，所以排序完毕。

blitsort函数是优化的归并算法，代码在include/sort.h中

sort函数是基础的归并排序

在tensor_transpose.cpp中代码执行在66391行开始读取tensor文件，在执行./run后生成build，需要的输入为 ./tensor_5 目标顺序 目标顺序的排序位
下面是01234 -->21043 的输入输出例子,例子是k5的桶排序和计数排序结合
![image](https://github.com/xuwaa/taco/blob/master/%E5%9B%BE%E7%89%871.png)

修改了transpose_coo_21043_k3,在桶内使用了归并排序的结果
![image](https://github.com/xuwaa/taco/blob/master/%E5%9B%BE%E7%89%872.png)
下面是transpose_coo_21043_k3,在桶内使用了原始的归并排序的结果
![image](https://github.com/xuwaa/taco/blob/master/%E5%9B%BE%E7%89%873.png)

对比可得到时间优劣

