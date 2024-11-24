import sys
from transformers import GPT2LMHeadModel, GPT2Tokenizer

tokenizer = GPT2Tokenizer.from_pretrained('gpt2')
model = GPT2LMHeadModel.from_pretrained('gpt2')


airline_service_prompt = (
    "Welcome to our Airline Services Chatbot! We're here to assist you with all your flight-related inquiries. "
    "Please provide us with your query, and we'll do our best to help you."
)
 
user_question = sys.argv[1]
full_prompt = airline_service_prompt + "\nUser: " + user_question+ "\nbot:"
input_ids = tokenizer.encode(full_prompt, return_tensors='pt')
output = model.generate(input_ids, max_new_tokens = 100, pad_token_id=tokenizer.eos_token_id, no_repeat_ngram_size=3, num_return_sequences=1, top_p = 0.95, temperature = 0.1, do_sample = True)
gen_text = tokenizer.decode(output[0], skip_special_tokens=True)
ans = ""
ans = gen_text.split("bot:")[1]
if("." in ans):
    ans = ans.split(".")[0]
if("\n" in ans):
    ans = " ".join(ans.split("\n"))

print("gpt2bot>"+ans)