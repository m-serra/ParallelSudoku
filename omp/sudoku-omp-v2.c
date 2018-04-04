#include "sudoku-aux.h"
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <time.h>
#include "string.h"

#define UNASSIGNED 0
#define UNCHANGEABLE -1
#define ROW(i) i/m_size
#define COL(i) i%m_size

struct node{
  int iteration, val;
  struct node *next;
};


int r_size, m_size, v_size;
struct node* add_node(struct node*, int, int);
int* read_matrix(char *argv[]);
int exists_in( int index, int* mask, int num);
int solve(int* sudoku, int* rows_mask, int* cols_mask, int* boxes_mask);
int new_mask( int size);
int int_to_mask(int num);
void init_masks(int* sudoku, int* rows_mask, int* cols_mask, int* boxes_mask);
void update_masks(int num, int row, int col, int* rows_mask, int* cols_mask, int* boxes_mask);
int is_safe_num( int* rows_mask, int* cols_mask, int* boxes_mask, int row, int col, int num);
void rm_num_masks(int num, int row, int col, int* rows_mask, int* cols_mask, int* boxes_mask);
void print_sudoku(int* sudoku);

int main(int argc, char *argv[]) {
    clock_t start, end;
    int result;
    double time;
    int* sudoku;

    if(argc == 2){
        sudoku = read_matrix(argv);

        int* rows_mask = (int*) malloc(m_size * sizeof(int));
        int* cols_mask = (int*) malloc(m_size * sizeof(int));
        int* boxes_mask = (int*) malloc(m_size * sizeof(int));

        printf("\n\ninitial sudoku:");
        print_sudoku(sudoku);

        //start measurement
        start = clock();

        init_masks(sudoku, rows_mask, cols_mask, boxes_mask);
        result = solve(sudoku, rows_mask, cols_mask, boxes_mask);

        // end measurement
        end = clock();

        printf("result sudoku:");
        print_sudoku(sudoku);

        verify_sudoku(sudoku, r_size) == 1 ? printf("corret, ") : printf("wrong, ");
        result == 1 ? printf("solved!\n") : printf("no solution!\n");

        time = (double) (end-start)/CLOCKS_PER_SEC;
        printf("took %lf sec (%.3lf ms).\n\n", time, time*1000.0);

        free(rows_mask);
        free(cols_mask);
        free(boxes_mask);
        free(sudoku);
    }else
        printf("invalid input arguments.\n");

    return 0;
}



int solve(int* sudoku, int* rows_mask, int* cols_mask, int* boxes_mask) {
    int i_in = 0, i = 0, val, zeros = 1, flag_back = 0, i_aux = 0, row, col, val_aux, is_safe =0 , allow =0;
    int cp_sudoku[v_size];
    struct node* head;
    head = NULL;
    // init cp_sudoku
    for(i = 0; i < v_size; i++){
        if(sudoku[i])
            cp_sudoku[i] = UNCHANGEABLE;
        else
            cp_sudoku[i] = UNASSIGNED;
    }

    while(zeros){
        zeros = 0;

        i = 0;
        if(flag_back){
            // search nearest element(on their left)

            while(head != NULL){ //stop going back if the list is empty
              /*printf("it: %d , val: %d\n", head->iteration, head->val);
              sleep(1);*/

                //há um erro, voltar atrás com as hipóteses criadas
                if(cp_sudoku[head->iteration] > 0 && cp_sudoku[head->iteration] <= m_size){
                    row = ROW(head->iteration);
                    col = COL(head->iteration);
                    //printf("row: %d, col: %d, val: %d\n",row, col, head->val );
                    val_aux = cp_sudoku[head->iteration] + 1;
                    rm_num_masks(cp_sudoku[head->iteration], row, col, rows_mask, cols_mask, boxes_mask);
                    sudoku[head->iteration] = UNASSIGNED;

                    if(cp_sudoku[head->iteration] < m_size){
                        cp_sudoku[head->iteration] = UNASSIGNED;
                        break;
                    }else{
                        cp_sudoku[head->iteration] = UNASSIGNED;
                        zeros ++;
                    }


                }
                printf("row: %d, col: %d, val: %d\n",ROW(head->iteration), COL(head->iteration), head->val );
                head = head->next;

            }

            if(head == NULL) //se a lista ficou vazia então não existe solução
                return 0; 
            else 
              i_in = head->iteration; //se não estiver, recomeçamos as hipoteses onde ficámos
        }

        #pragma omp parallel for private(row, col, val) reduction(max:i_aux)
        for(i = i_in; i < v_size; i++){ //iterate the sudoku vector cells
            #pragma omp critical
            allow = (cp_sudoku[i] == UNASSIGNED || flag_back); 
            if(allow){ //we only place a new hypothesis if the present cell has a 0 or if there was a mistake in the previous attempt
                row = ROW(i);
                col = COL(i);

                val = 1;
                #pragma omp critical
                {
                  if(flag_back){
                      val = val_aux;
                      flag_back = 0;
                  }
                }
                #pragma omp atomic
                zeros++;

                for(; val <= m_size; val++){  //for every cell try every number
                  #pragma omp critical
                  is_safe = is_safe_num( rows_mask, cols_mask, boxes_mask, row, col, val); //if the number isn't safe skip it
                    if(is_safe){
                      #pragma omp critical
                        {
                          head = add_node(head, i, val); //add the new hypothesis to the list
                          printf("LOOP - row: %d, col: %d, val: %d\n",ROW(head->iteration), COL(head->iteration), head->val );
                          cp_sudoku[i] = val; 
                          sudoku[i] = val;
                          update_masks(val, row, col, rows_mask, cols_mask, boxes_mask);
                        /*  print_sudoku(sudoku);
                          sleep(1);*/
                        }

                          //sleep(1);
                        //} 
                        break; //if a number was placed on the current cell go to next cell
                    }else if(val == m_size){ //if no number can be placed on the cell there must be a mistake 
                      #pragma omp critical
                        flag_back = 1;
                        i_aux = i;
                        i = v_size; //break
                    }
                }
            }
        }
       printf("%d\n", zeros);
      }
    return 1;
}
struct node* add_node(struct node *n, int iteration, int val){
  struct node *temp;
  temp = (struct node*) calloc(1, sizeof(struct node));
  temp->iteration = iteration;
  temp->val = val;
  if(n == NULL){
    temp->next = NULL;

  }else{
    temp->next = n;
  }
  return temp;
}

int exists_in(int index, int* mask, int num) {
    int res, masked_num = int_to_mask(num);

    res = mask[index] | masked_num;
    if(res != mask[index])
        return 0;
    return 1;
}

int is_safe_num(int* rows_mask, int* cols_mask, int* boxes_mask, int row, int col, int num) {
    return !exists_in(row, rows_mask, num) && !exists_in(col, cols_mask, num) && !exists_in(r_size*(row/r_size)+col/r_size, boxes_mask, num);
}

int new_mask(int size) {
    return (0 << (size-1));
}

int int_to_mask(int num) {
    return (1 << (num-1));
}

void init_masks(int* sudoku, int* rows_mask, int* cols_mask, int* boxes_mask) {
    int i, mask, row, col;

    for(i = 0; i < m_size; i++){
        rows_mask[i]  = UNASSIGNED;
        cols_mask[i]  = UNASSIGNED;
        boxes_mask[i] = UNASSIGNED;
    }

    for(i = 0; i < v_size; i++){
        if(sudoku[i]){
            row = ROW(i);
            col = COL(i);

            mask = int_to_mask(sudoku[i]);          //convert number found to mask ex: if dim=4x4, 3 = 0010
            rows_mask[row] = rows_mask[row] | mask; //to add the new number to the current row's mask use bitwise OR
            cols_mask[col] = cols_mask[col] | mask;
            boxes_mask[r_size*(row/r_size)+col/r_size] = boxes_mask[r_size*(row/r_size)+col/r_size] | mask;
        }
    }
}

void rm_num_masks(int num, int row, int col, int* rows_mask, int* cols_mask, int* boxes_mask) {
    int num_mask = int_to_mask(num);
    rows_mask[row] = rows_mask[row] ^ num_mask;
    cols_mask[col] = cols_mask[col] ^ num_mask;
    boxes_mask[r_size*(row/r_size)+col/r_size] = boxes_mask[r_size*(row/r_size)+col/r_size] ^ num_mask;
}

void update_masks(int num, int row, int col, int* rows_mask, int* cols_mask, int* boxes_mask) {
    int new_mask = int_to_mask(num);
    rows_mask[row] = rows_mask[row] | new_mask;
    cols_mask[col] = cols_mask[col] | new_mask;
    boxes_mask[r_size*(row/r_size)+col/r_size] = boxes_mask[r_size*(row/r_size)+col/r_size] | new_mask;
}

int* read_matrix(char *argv[]) {
    FILE *fp;
    size_t characters, len = 1;
    char *line = NULL, aux[3];
    int i, j, k, l;

    //verifies if the file was correctly opened
    if((fp = fopen(argv[1], "r+")) == NULL) {
        fprintf(stderr, "unable to open file %s\n", argv[1]);
        exit(1);
    }

    getline(&line, &len, fp);
    r_size = atoi(line);
    m_size = r_size *r_size;
    v_size = m_size * m_size;

    int* sudoku = (int*)malloc(v_size * sizeof(int));

    k = 0, l = 0;
    len = m_size * 2;
    for(i = 0; (characters = getline(&line, &len, fp)) != -1; i++){
        for (j = 0; j < characters; j++) {
            if(isdigit(line[j])){
                aux[l++] = line[j];
            }else if(l > 0){
                aux[l] = '\0';
                l = 0;
                sudoku[k++] = atoi(aux);
                memset(aux, 0, sizeof aux);
            }
        }
    }

    free(line);
    fclose(fp);

    return sudoku;
}

void print_sudoku(int *sudoku) {
    int i;

    printf("\n\n");
    for (i = 0; i < v_size; i++) {
        if(i%m_size != m_size - 1){
            if(i%m_size%r_size == 0 && i%m_size != 0)
                printf("|");
            printf("%2d ", sudoku[i]);
        }else{
            printf("%2d\n", sudoku[i]);
            if(((i/m_size)+1)%r_size == 0)
                printf("\n");

        }
    }
    printf("\n\n");
}
